// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Game.h"
#include "Factions.h"
#include "Space.h"
#include "Player.h"
#include "Body.h"
#include "SpaceStation.h"
#include "HyperspaceCloud.h"
#include "Pi.h"
#include "ShipCpanel.h"
#include "Sfx.h"
#include "MathUtil.h"
#include "SectorView.h"
#include "WorldView.h"
#include "DeathView.h"
#include "GalacticView.h"
#include "SystemView.h"
#include "SystemInfoView.h"
#include "UIView.h"
#include "LuaEvent.h"
#include "ObjectViewerView.h"
#include "FileSystem.h"
#include "graphics/Renderer.h"
#include "graphics/gl3/Effect.h"
#include "ui/Context.h"

static const char s_saveStart[]   = "PARAGON";
static const char s_saveEnd[]     = "END";

int Game::s_loadedGameVersion = s_latestSaveVersion;

Game::Game(const SystemPath &path, double time) :
	m_time(time),
	m_state(STATE_NORMAL),
	m_wantHyperspace(false),
	m_wantPhaseMode(false),
	m_timeAccel(TIMEACCEL_1X),
	m_requestedTimeAccel(TIMEACCEL_1X),
	m_forceTimeAccel(false)
{
	Pi::FlushCaches();

	m_space.reset(new Space(this, path));
	SpaceStation *station = static_cast<SpaceStation*>(m_space->FindBodyForPath(&path));
	assert(station);

	m_player.reset(new Player("kanara"));

	m_space->AddBody(m_player.get());

	m_player->SetFrame(station->GetFrame());
	m_player->SetDockedWith(station, 0);

	Polit::Init();

	CreateViews();
}

Game::Game(const SystemPath &path, const vector3d &pos, double time) :
	m_time(time),
	m_state(STATE_NORMAL),
	m_wantHyperspace(false),
	m_wantPhaseMode(false),
	m_timeAccel(TIMEACCEL_1X),
	m_requestedTimeAccel(TIMEACCEL_1X),
	m_forceTimeAccel(false)
{
	Pi::FlushCaches();

	m_space.reset(new Space(this, path));
	Body *b = m_space->FindBodyForPath(&path);
	assert(b);

	m_player.reset(new Player("kanara"));

	m_space->AddBody(m_player.get());

	m_player->SetFrame(b->GetFrame());

	m_player->SetPosition(pos);
	m_player->SetVelocity(vector3d(0,0,0));

	Polit::Init();

	CreateViews();
}

Game::~Game()
{
	DestroyViews();

	// XXX this shutdown sequence is critical:
	// 1- RemoveBody marks the Player for removal from Space,
	// 2- Space is destroyed, which actually goes through its removal list,
	//    removes the Player from Space and calls SetFrame(0) on it, which unlinks
	//    any references it has to other Space items
	// 3- Player is destroyed
	//
	// note that because of the declaration order of m_space and m_player in Game,
	// without these explicit Reset() calls, m_player would be deleted before m_space is,
	// which causes problems because then when Space is destroyed it tries to reference
	// the deleted Player object (to call SetFrame(0))
	//
	// ideally we'd split Player out into two objects, the physics part that
	// is owned by the space, and the game part that holds all the player
	// attributes and whatever else

	m_space->RemoveBody(m_player.get());
	m_space.reset();
	m_player.reset();
}

Game::Game(Serializer::Reader &rd) :
	m_timeAccel(TIMEACCEL_1X),
	m_requestedTimeAccel(TIMEACCEL_1X),
	m_forceTimeAccel(false)
{
	// signature check
	for (Uint32 i = 0; i < strlen(s_saveStart)+1; i++)
		if (rd.Byte() != s_saveStart[i]) throw SavedGameCorruptException();

	// version check
	rd.SetStreamVersion(rd.Int32());
	Output("savefile version: %d\n", rd.StreamVersion());
	if (rd.StreamVersion() < s_baseSaveVersion) {
		//Output("Can't load savefile, expected version: >= %d\n", s_baseSaveVersion);
		Output("Unfortunately, this save file version (%d) is no longer supported after latest update\nCan't load savefile, expected version: >= %d\n",
			rd.StreamVersion(), s_latestSaveVersion);
		throw SavedGameWrongVersionException();
	}
	s_loadedGameVersion = rd.StreamVersion();

	//--------------------- PATCH 76 #1 (DISABLED)
	// Version 76 changes: +small starport upgraded to have more landing pads (LUA)
	/*bool v76_patch = false;
	SpaceStationType* v76_sst;			// space station type
	SpaceStationType::TBayGroups v76_bg;// bay groups
	unsigned int v76_ndp;				// num docking ports
	if (s_loadedGameVersion < s_latestSaveVersion) {
		// Patch station type: starport_small
		std::vector<SpaceStationType>& surface_stations = SpaceStationType::surfaceStationTypes;
		for (unsigned int i = 0; i < surface_stations.size(); i++) {
			if (surface_stations[i].modelName == "starport_small") {
				v76_sst = &surface_stations[i];
				v76_bg = surface_stations[i].bayGroups;
				v76_ndp = surface_stations[i].numDockingPorts;
				while (surface_stations[i].bayGroups.size() > 4) {
					surface_stations[i].bayGroups.pop_back();
				}
				surface_stations[i].numDockingPorts = 4;
				v76_patch = true;
			}
		}
	}*/
	//---------------------- PATCH 76 #1 END

	// XXX This must be done after loading sectors once we can change them in game
	Pi::FlushCaches();

	Serializer::Reader section;

	// game state
	section = rd.RdSection("Game");
	m_time = section.Double();
	m_state = State(section.Int32());

	m_wantHyperspace = section.Bool();
	//------------------------- SAVE PATCH 79
	if(s_loadedGameVersion >= 79) {	
		m_wantPhaseMode = section.Bool();
	} else {
		m_wantPhaseMode = false;
	}
	//------------------------- PATCH 79 END
	m_hyperspaceProgress = section.Double();
	m_hyperspaceDuration = section.Double();
	m_hyperspaceEndTime = section.Double();

	// space, all the bodies and things
	section = rd.RdSection("Space");
	m_space.reset(new Space(this, section, m_time));
	m_player.reset(static_cast<Player*>(m_space->GetBodyByIndex(section.Int32())));

	// space transition state
	section = rd.RdSection("HyperspaceClouds");

	// hyperspace clouds being brought over from the previous system
	Uint32 nclouds = section.Int32();
	for (Uint32 i = 0; i < nclouds; i++) {
		m_hyperspaceClouds.push_back(static_cast<HyperspaceCloud*>(Body::Unserialize(section, 0)));
    }

	// system political stuff
	section = rd.RdSection("Polit");
	Polit::Unserialize(section);


	// views
	LoadViews(rd);


	// lua
	section = rd.RdSection("LuaModules");
	Pi::luaSerializer->Unserialize(section);


	// signature check
	for (Uint32 i = 0; i < strlen(s_saveEnd)+1; i++)
		if (rd.Byte() != s_saveEnd[i]) throw SavedGameCorruptException();

	//--------------------- PATCH 76 #2 (DISABLED)
	// Version 76 changes: +small starport upgraded to have more landing pads (LUA)
	/*if (v76_patch) {
		v76_sst->bayGroups = v76_bg;
		v76_sst->numDockingPorts = v76_ndp;
	}*/
	//---------------------- PATCH 76 #2 END
}

void Game::Serialize(Serializer::Writer &wr)
{
	// leading signature
	for (Uint32 i = 0; i < strlen(s_saveStart)+1; i++)
		wr.Byte(s_saveStart[i]);

	// version
	wr.Int32(s_latestSaveVersion);

	Serializer::Writer section;

	// game state
	section.Double(m_time);
	section.Int32(Uint32(m_state));

	section.Bool(m_wantHyperspace);
	section.Bool(m_wantPhaseMode);
	section.Double(m_hyperspaceProgress);
	section.Double(m_hyperspaceDuration);
	section.Double(m_hyperspaceEndTime);

	wr.WrSection("Game", section.GetData());


	// space, all the bodies and things
	section = Serializer::Writer();
	m_space->Serialize(section);
	section.Int32(m_space->GetIndexForBody(m_player.get()));
	wr.WrSection("Space", section.GetData());


	// space transition state
	section = Serializer::Writer();

	// hyperspace clouds being brought over from the previous system
	section.Int32(m_hyperspaceClouds.size());
    for (std::list<HyperspaceCloud*>::const_iterator i = m_hyperspaceClouds.begin(); i != m_hyperspaceClouds.end(); ++i) {
		(*i)->Serialize(section, m_space.get());
    }

	wr.WrSection("HyperspaceClouds", section.GetData());


	// system political data (crime etc)
	section = Serializer::Writer();
	Polit::Serialize(section);
	wr.WrSection("Polit", section.GetData());


	// views. must be saved in init order
	section = Serializer::Writer();
	Pi::cpan->Save(section);
	wr.WrSection("ShipCpanel", section.GetData());

	section = Serializer::Writer();
	Pi::sectorView->Save(section);
	wr.WrSection("SectorView", section.GetData());

	section = Serializer::Writer();
	Pi::worldView->Save(section);
	wr.WrSection("WorldView", section.GetData());


	// lua
	section = Serializer::Writer();
	Pi::luaSerializer->Serialize(section);
	wr.WrSection("LuaModules", section.GetData());


	// trailing signature
	for (Uint32 i = 0; i < strlen(s_saveEnd)+1; i++)
		wr.Byte(s_saveEnd[i]);
}

void Game::TimeStep(float step)
{
	PROFILE_SCOPED()
	m_time += step;			// otherwise planets lag time accel changes by a frame
	if (m_state == STATE_HYPERSPACE && Pi::game->GetTime() >= m_hyperspaceEndTime)
		m_time = m_hyperspaceEndTime;

	m_space->TimeStep(step);

	// XXX ui updates, not sure if they belong here
	Pi::cpan->TimeStepUpdate(step);
	Sfx::TimeStepAll(step, m_space->GetRootFrame());
	log->Update(m_timeAccel == Game::TIMEACCEL_PAUSED);

	if (m_state == STATE_HYPERSPACE) {
		if (Pi::game->GetTime() >= m_hyperspaceEndTime) {
			SwitchToNormalSpace();
			m_player->EnterSystem();
			RequestTimeAccel(TIMEACCEL_1X);
		}
		else
			m_hyperspaceProgress += step;
		return;
	}

	if (m_wantHyperspace) {
		assert(m_state == STATE_NORMAL);
		SwitchToHyperspace();
		return;
	}

	Graphics::GL3::Effect::StaticUpdate(step);
}

bool Game::UpdateTimeAccel()
{
	PROFILE_SCOPED()
	// don't modify the timeaccel if the game is paused
	if (m_requestedTimeAccel == Game::TIMEACCEL_PAUSED) {
		SetTimeAccel(Game::TIMEACCEL_PAUSED);
		return false;
	}

	TimeAccel newTimeAccel = m_requestedTimeAccel;

	// ludicrous speed
	if (m_player->GetFlightState() == Ship::HYPERSPACE) {
		newTimeAccel = Game::TIMEACCEL_1X; //Game::TIMEACCEL_HYPERSPACE;
		RequestTimeAccel(newTimeAccel);
	}

	// force down to timeaccel 1 during the docking sequence or when just initiating hyperspace
	else if (m_player->GetFlightState() == Ship::DOCKING || m_player->GetFlightState() == Ship::JUMPING) {
		newTimeAccel = std::min(newTimeAccel, Game::TIMEACCEL_10X);
		RequestTimeAccel(newTimeAccel);
	}

	// normal flight
	else if (m_player->GetFlightState() == Ship::FLYING) {

		// special timeaccel lock rules while in alert
		if (m_player->GetAlertState() == Ship::ALERT_SHIP_NEARBY)
			newTimeAccel = std::min(newTimeAccel, Game::TIMEACCEL_10X);
		else if (m_player->GetAlertState() == Ship::ALERT_SHIP_FIRING)
			newTimeAccel = std::min(newTimeAccel, Game::TIMEACCEL_1X);

		else if (!m_forceTimeAccel) {
			// check we aren't too near to objects for timeaccel //
			for (const Body* b : m_space->GetBodies()) {
				if (b == m_player.get()) continue;
				if (b->IsType(Object::HYPERSPACECLOUD)) continue;

				vector3d toBody = m_player->GetPosition() - b->GetPositionRelTo(m_player->GetFrame());
				double dist = toBody.Length();
				double rad = b->GetPhysRadius();

				if (dist < 1000.0) {
					newTimeAccel = std::min(newTimeAccel, Game::TIMEACCEL_1X);
				} else if (dist < std::min(rad+0.0001*AU, rad*1.1)) {
					newTimeAccel = std::min(newTimeAccel, Game::TIMEACCEL_10X);
				} else if (dist < std::min(rad+0.001*AU, rad*5.0)) {
					newTimeAccel = std::min(newTimeAccel, Game::TIMEACCEL_100X);
				} else if (dist < std::min(rad+0.01*AU,rad*10.0)) {
					newTimeAccel = std::min(newTimeAccel, Game::TIMEACCEL_1000X);
				} else if (dist < std::min(rad+0.1*AU, rad*1000.0)) {
					newTimeAccel = std::min(newTimeAccel, Game::TIMEACCEL_10000X);
				}
			}

			if (!m_player->AIIsActive()) {		// don't do this when autopilot is active
				const double locVel = m_player->GetAngVelocity().Length();
				const double strictness = 20.0;
				if(locVel > strictness / Game::s_timeAccelRates[TIMEACCEL_10X]) {
					newTimeAccel = std::min(newTimeAccel, Game::TIMEACCEL_1X);
				} else if(locVel > strictness / Game::s_timeAccelRates[TIMEACCEL_100X]) {
					newTimeAccel = std::min(newTimeAccel, Game::TIMEACCEL_10X);
				} else if(locVel > strictness / Game::s_timeAccelRates[TIMEACCEL_1000X]) {
					newTimeAccel = std::min(newTimeAccel, Game::TIMEACCEL_100X);
				} else if(locVel > strictness / Game::s_timeAccelRates[TIMEACCEL_10000X]) {
					newTimeAccel = std::min(newTimeAccel, Game::TIMEACCEL_1000X);
				}
			}
		}
	}

    if (m_isInFastTravel)
        newTimeAccel = Game::TIMEACCEL_10X;

	// no change
	if (newTimeAccel == m_timeAccel)
		return false;

	SetTimeAccel(newTimeAccel);
	return true;
}

void Game::WantHyperspace(bool phase_mode)
{
	assert(m_state == STATE_NORMAL);
	m_wantHyperspace = true;
	m_wantPhaseMode = phase_mode;
}

double Game::GetHyperspaceArrivalProbability() const
{
	double progress = m_hyperspaceProgress / m_hyperspaceDuration;
	const double fudge = 4.0;
	const double scale = 1.0 / (1.0 - exp(-fudge));
	return scale * (1.0 - exp(-fudge * progress));
}

void Game::SwitchToHyperspace()
{
	PROFILE_SCOPED()
	// remember where we came from so we can properly place the player on exit
	m_hyperspaceSource = m_space->GetStarSystem()->GetPath();
	m_hyperspaceDest =  m_player->GetHyperspaceDest();
	m_hyperspacePhaseMode = m_wantPhaseMode;

	// find all the departure clouds, convert them to arrival clouds and store
	// them for the next system
	m_hyperspaceClouds.clear();

	for (Body* b : m_space->GetBodies()) {

		if (!b->IsType(Object::HYPERSPACECLOUD)) continue;

		// only want departure clouds with ships in them
		HyperspaceCloud *cloud = static_cast<HyperspaceCloud*>(b);
		if (!cloud->SupportsDeparture() || cloud->GetShip() == 0)
			continue;

		// make sure they're going to the same place as us
		if (!m_hyperspaceDest.IsSameSystem(cloud->GetShip()->GetHyperspaceDest()))
			continue;

		// remove it from space
		m_space->RemoveBody(cloud);

		// player and the clouds are coming to the next system, but we don't
		// want the player to have any memory of what they were (we're just
		// reusing them for convenience). tell the player it was deleted so it
		// can clean up
		m_player->NotifyRemoved(cloud);

		// turn the cloud arround
		cloud->GetShip()->SetHyperspaceDest(m_hyperspaceSource);
		cloud->SetCloudType(EHCT_ARRIVAL);

		// and remember it
		m_hyperspaceClouds.push_back(cloud);
	}

	Output(SIZET_FMT " clouds brought over\n", m_hyperspaceClouds.size());

	// remove the player from space
	m_space->RemoveBody(m_player.get());

	// create hyperspace :)
	m_space.reset(new Space(this));

	//m_space->GetBackground()->SetDrawFlags( Background::Container::DRAW_STARS );

	// put the player in it
	m_player->SetFrame(m_space->GetRootFrame());
	m_space->AddBody(m_player.get());

	// put player at the origin. kind of unnecessary since it won't be moving
	// but at least it gives some consistency
	m_player->SetPosition(vector3d(0,0,0));
	m_player->SetVelocity(vector3d(0,0,0));
	m_player->SetOrient(matrix3x3d::Identity());

	// animation and end time counters
	m_hyperspaceProgress = 0;
	m_hyperspaceDuration = m_player->GetHyperspaceDuration();
	m_hyperspaceEndTime = Pi::game->GetTime() + m_hyperspaceDuration;

	m_state = STATE_HYPERSPACE;
	m_wantHyperspace = false;

	Output("Started hyperspacing...\n");
}

void Game::SwitchToNormalSpace()
{
	PROFILE_SCOPED()
	// remove the player from hyperspace
	m_space->RemoveBody(m_player.get());

	// create a new space for the system
	m_space.reset(new Space(this, m_hyperspaceDest));

	// put the player in it
	m_player->SetFrame(m_space->GetRootFrame());
	m_space->AddBody(m_player.get());

	// Non-phase mode: Normal departure cloud
	// Phase mode: Phase permanent cloud
	HyperspaceCloud* target_perma_cloud = nullptr;
	if(m_hyperspacePhaseMode) {
		target_perma_cloud = m_space->GetFreePermaHyperspaceCloud();
	}

	// place it
	if(!target_perma_cloud) {
		m_player->SetPosition(m_space->GetHyperspaceExitPoint(m_hyperspaceSource, m_hyperspaceDest));
	} else {
		m_player->SetFrame(target_perma_cloud->GetFrame());
		m_player->SetPosition(target_perma_cloud->GetPosition());
	}
	m_player->SetVelocity(vector3d(0,0,-100.0));
	m_player->SetOrient(matrix3x3d::Identity());

	HyperspaceCloud* cloud = nullptr;
	if(!target_perma_cloud) {
		// place the exit cloud
		cloud = m_space->CreateHyperspaceCloud(0, Pi::game->GetTime(), EHCT_ARRIVAL);
		cloud->SetFrame(m_space->GetRootFrame());
		cloud->SetPosition(m_player->GetPosition());
		m_space->AddBody(cloud);
	}

	for (std::list<HyperspaceCloud*>::iterator i = m_hyperspaceClouds.begin(); i != m_hyperspaceClouds.end(); ++i) {
		cloud = *i;

		// TODO: This should handle other permanent clouds transporting NPC ships (Salwan) (#166)
		if (cloud->IsPermanent()) continue;

		cloud->SetFrame(m_space->GetRootFrame());
		cloud->SetPosition(m_space->GetHyperspaceExitPoint(m_hyperspaceSource, m_hyperspaceDest));

		m_space->AddBody(cloud);

		if (cloud->GetDueDate() < Pi::game->GetTime()) {
			// they emerged from hyperspace some time ago
			Ship *ship = cloud->EvictShip();

			ship->SetFrame(m_space->GetRootFrame());
			ship->SetVelocity(vector3d(0,0,-100.0));
			ship->SetOrient(matrix3x3d::Identity());
			ship->SetFlightState(Ship::FLYING);

			const SystemPath &sdest = ship->GetHyperspaceDest();
			if (sdest.IsSystemPath()) {
				// travelling to the system as a whole, so just dump them on
				// the cloud - we can't do any better in this case
				ship->SetPosition(cloud->GetPosition());
			} else {
				// on their way to a body. they're already in-system so we
				// want to simulate some travel to their destination. we
				// naively assume full accel for half the distance, flip and
				// full brake for the rest.
				Body *target_body = m_space->FindBodyForPath(&sdest);
				double dist_to_target = cloud->GetPositionRelTo(target_body).Length();
				double half_dist_to_target = dist_to_target / 2.0;
				double accel = -(ship->GetShipType()->linThrust[ShipType::THRUSTER_FORWARD] / 
					ship->GetMass());
				double travel_time = Pi::game->GetTime() - cloud->GetDueDate();

				// I can't help but feel some actual math would do better here
				double speed = 0;
				double dist = 0;
				while (travel_time > 0 && dist <= half_dist_to_target) {
					speed += accel;
					dist += speed;
					travel_time--;
				}
				while (travel_time > 0 && dist < dist_to_target) {
					speed -= accel;
					dist += speed;
					travel_time--;
				}

				if (travel_time <= 0) {
					vector3d pos =
						target_body->GetPositionRelTo(m_space->GetRootFrame()) +
						cloud->GetPositionRelTo(target_body).Normalized() * (dist_to_target - dist);
					ship->SetPosition(pos);
				} else {
					// ship made it with time to spare. just put it somewhere
					// near the body. the script should be issuing a dock or
					// flyto command in onEnterSystem so it should sort it
					// itself out long before the player can get near

					SystemBody *sbody = m_space->GetStarSystem()->GetBodyByPath(&sdest);
					if (sbody->GetType() == SystemBody::TYPE_STARPORT_ORBITAL) {
						ship->SetFrame(target_body->GetFrame());
						ship->SetPosition(MathUtil::RandomPointOnSphere(1000.0)*1000.0); // somewhere 1000km out
					}

					else {
						if (sbody->GetType() == SystemBody::TYPE_STARPORT_SURFACE) {
							sbody = sbody->GetParent();
							SystemPath path = m_space->GetStarSystem()->GetPathOf(sbody);
							target_body = m_space->FindBodyForPath(&path);
						}

						double sdist = sbody->GetRadius()*2.0;

						ship->SetFrame(target_body->GetFrame());
						ship->SetPosition(MathUtil::RandomPointOnSphere(sdist));
					}
				}
			}

			m_space->AddBody(ship);

			LuaEvent::Queue("onEnterSystem", ship);
		}
	}
	m_hyperspaceClouds.clear();

	//m_space->GetBackground()->SetDrawFlags( Background::Container::DRAW_SKYBOX | Background::Container::DRAW_STARS );

	m_state = STATE_NORMAL;
}

const float Game::s_timeAccelRates[] = {
	0.0f,       // paused
	1.0f,       // 1x
	10.0f,      // 10x
	100.0f,     // 100x
	1000.0f,    // 1000x
	10000.0f,   // 10000x
	100000.0f   // hyperspace
};

const float Game::s_timeInvAccelRates[] = {
	0.0f,       // paused
	1.0f,       // 1x
	0.1f,      // 10x
	0.01f,     // 100x
	0.001f,    // 1000x
	0.0001f,   // 10000x
	0.00001f   // hyperspace
};

void Game::SetTimeAccel(TimeAccel t)
{
	// don't want player to spin like mad when hitting time accel
	if ((t != m_timeAccel) && (t > TIMEACCEL_1X) &&
			m_player->GetPlayerController()->GetRotationDamping()) {
		m_player->SetAngVelocity(vector3d(0,0,0));
		m_player->SetTorque(vector3d(0,0,0));
		m_player->SetAngThrusterState(vector3d(0.0));
	}

	// Give all ships a half-step acceleration to stop autopilot overshoot
	if (t < m_timeAccel)
		for (Body* b : m_space->GetBodies())
			if (b->IsType(Object::SHIP))
				(static_cast<Ship*>(b))->TimeAccelAdjust(0.5f * GetTimeStep());

	m_timeAccel = t;

	if (m_timeAccel == TIMEACCEL_PAUSED || m_timeAccel == TIMEACCEL_HYPERSPACE) {
		m_requestedTimeAccel = m_timeAccel;
		m_forceTimeAccel = false;
	}
}

void Game::RequestTimeAccel(TimeAccel t, bool force)
{
	m_requestedTimeAccel = t;
	m_forceTimeAccel = force;
}

// XXX this should be in some kind of central UI management class that
// creates a set of UI views held by the game. right now though the views
// are rather fundamentally tied to their global points and assume they
// can all talk to each other. given the difficulty of disentangling all
// that and the impending move to Rocket, its better right now to just
// manage creation and destruction here to get the timing and order right
void Game::CreateViews()
{
	Pi::SetView(0);

	// XXX views expect Pi::game and Pi::player to exist
	Pi::game = this;
	Pi::player = m_player.get();

	Pi::cpan = new ShipCpanel(Pi::renderer);
	Pi::sectorView = new SectorView();
	Pi::worldView = new WorldView();
	Pi::galacticView = new GalacticView();
	Pi::systemView = new SystemView();
	Pi::systemInfoView = new SystemInfoView();
	Pi::spaceStationView = new UIView("StationView");
	Pi::infoView = new UIView("InfoView");
	Pi::deathView = new DeathView();
	Pi::settingsView = new UIView("SettingsInGame");

	// view manager will handle setting this probably
	Pi::galacticView->SetRenderer(Pi::renderer);
	Pi::infoView->SetRenderer(Pi::renderer);
	Pi::sectorView->SetRenderer(Pi::renderer);
	Pi::systemInfoView->SetRenderer(Pi::renderer);
	Pi::systemView->SetRenderer(Pi::renderer);
	Pi::worldView->SetRenderer(Pi::renderer);
	Pi::deathView->SetRenderer(Pi::renderer);

#if WITH_OBJECTVIEWER
	Pi::objectViewerView = new ObjectViewerView();
	Pi::objectViewerView->SetRenderer(Pi::renderer);
#endif
	UI::Point scrSize = Pi::ui->GetContext()->GetSize();
	log = new GameLog(
		Pi::ui->GetContext()->GetFont(UI::Widget::FONT_SMALLEST),
		vector2f(scrSize.x, scrSize.y));
}

// XXX mostly a copy of CreateViews
void Game::LoadViews(Serializer::Reader &rd)
{
	Pi::SetView(0);

	// XXX views expect Pi::game and Pi::player to exist
	Pi::game = this;
	Pi::player = m_player.get();

	Serializer::Reader section = rd.RdSection("ShipCpanel");
	Pi::cpan = new ShipCpanel(section, Pi::renderer);

	section = rd.RdSection("SectorView");
	Pi::sectorView = new SectorView(section);

	section = rd.RdSection("WorldView");
	Pi::worldView = new WorldView(section);

	Pi::galacticView = new GalacticView();
	Pi::systemView = new SystemView();
	Pi::systemInfoView = new SystemInfoView();
	Pi::spaceStationView = new UIView("StationView");
	Pi::infoView = new UIView("InfoView");
	Pi::deathView = new DeathView();
	Pi::settingsView = new UIView("SettingsInGame");

#if WITH_OBJECTVIEWER
	Pi::objectViewerView = new ObjectViewerView();
	Pi::objectViewerView->SetRenderer(Pi::renderer);
#endif

	Pi::galacticView->SetRenderer(Pi::renderer);
	Pi::infoView->SetRenderer(Pi::renderer);
	Pi::sectorView->SetRenderer(Pi::renderer);
	Pi::systemInfoView->SetRenderer(Pi::renderer);
	Pi::systemView->SetRenderer(Pi::renderer);
	Pi::worldView->SetRenderer(Pi::renderer);
	Pi::deathView->SetRenderer(Pi::renderer);

	UI::Point scrSize = Pi::ui->GetContext()->GetSize();
	log = new GameLog(
		Pi::ui->GetContext()->GetFont(UI::Widget::FONT_SMALLEST),
		vector2f(scrSize.x, scrSize.y));
}

void Game::DestroyViews()
{
	Pi::SetView(0);

#if WITH_OBJECTVIEWER
	delete Pi::objectViewerView;
#endif

	delete Pi::settingsView;
	delete Pi::deathView;
	delete Pi::infoView;
	delete Pi::spaceStationView;
	delete Pi::systemInfoView;
	delete Pi::systemView;
	delete Pi::galacticView;
	delete Pi::worldView;
	delete Pi::sectorView;
	delete Pi::cpan;
	delete log;

	Pi::objectViewerView = 0;
	Pi::settingsView = 0;
	Pi::deathView = 0;
	Pi::infoView = 0;
	Pi::spaceStationView = 0;
	Pi::systemInfoView = 0;
	Pi::systemView = 0;
	Pi::galacticView = 0;
	Pi::worldView = 0;
	Pi::sectorView = 0;
	Pi::cpan = 0;
	log = 0;
}

Game *Game::LoadGame(const std::string &filename)
{
	Output("Game::LoadGame('%s')\n", filename.c_str());
	auto file = FileSystem::userFiles.ReadFile(FileSystem::JoinPathBelow(Pi::SAVE_DIR_NAME, filename));
	if (!file) throw CouldNotOpenFileException();
	Serializer::Reader rd(file->AsByteRange());
	return new Game(rd);
	// file data is freed here
}

void Game::SaveGame(const std::string &filename, Game *game)
{
	assert(game);
	if (!FileSystem::userFiles.MakeDirectory(Pi::SAVE_DIR_NAME)) {
		throw CouldNotOpenFileException();
	}

	Serializer::Writer wr;
	game->Serialize(wr);

	const std::string data = wr.GetData();

	FILE *f = FileSystem::userFiles.OpenWriteStream(FileSystem::JoinPathBelow(Pi::SAVE_DIR_NAME, filename));
	if (!f) throw CouldNotOpenFileException();

	size_t nwritten = fwrite(data.data(), data.length(), 1, f);
	fclose(f);

	if (nwritten != 1) throw CouldNotWriteToFileException();
}

void Game::EnumerateAllHyperspaceClouds(std::list<HyperspaceCloud*>& clouds_out) 
{
	for (Body* b : m_space->GetBodies()) {
		if (!b->IsType(Object::HYPERSPACECLOUD)) {
            continue;
        }
        clouds_out.push_back(static_cast<HyperspaceCloud*>(b));
	}
}

void Game::SetFastTravel(const bool value) {
    m_isInFastTravel = value;
}

const bool Game::GetFastTravel() const {
    return m_isInFastTravel;
}
