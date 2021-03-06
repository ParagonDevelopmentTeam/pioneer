// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _SHIPCPANELMULTIFUNCDISPLAYS_H
#define _SHIPCPANELMULTIFUNCDISPLAYS_H

#include "gui/Gui.h"
#include "EquipType.h"
#include "Serializer.h"
#include "Object.h"

class Body;
namespace Graphics { class Renderer; }

enum multifuncfunc_t {
	MFUNC_SCANNER,
	MFUNC_EQUIPMENT,
	MFUNC_MSGLOG,
	MFUNC_MAX
};

class IMultiFunc {
public:
	sigc::signal<void> onGrabFocus;
	sigc::signal<void> onUngrabFocus;
	virtual void Update() = 0;
};

class MsgLogWidget: public IMultiFunc, public Gui::Fixed {
public:
	MsgLogWidget();
	void GetSizeRequested(float size[2]);

	void ImportantMessage(const std::string &sender, const std::string &msg); 
	void Message(const std::string &sender, const std::string &msg);
	virtual void Update();
private:
	enum Type {
		NONE = -1,
		NOT_IMPORTANT = 0,
		MUST_SEE = 1
	};
	void ShowNext();
	struct message_t {
		message_t(std::string s, std::string m, Type t): sender(s), message(m), type(t) {}
		std::string sender;
		std::string message;
		Type type;

		inline bool operator==(const message_t& other) const {
			if(message.size() == other.message.size() &&
				sender == other.sender &&
				message == other.message) 
			{
				return true;
			} else {
				return false;
			}
		}
		inline bool operator!=(const message_t& other) const {
			return !(*this == other);
		}
		inline bool IsEqual(const std::string& _sender, const std::string& _message) const {
			if (message.size() == _message.size() &&
				sender == _sender &&
				message == _message)
			{
				return true;
			} else {
				return false;
			}
		}
	};

	std::list<message_t> m_msgQueue;
	Uint32 m_msgAge;
	std::vector<Gui::Label*> m_msgLabels;
	std::vector<std::string> m_typeTexts;
	int m_lineCount;
	Type m_curMsgType;
};

class ScannerWidget: public IMultiFunc, public Gui::Widget {
public:
	ScannerWidget(Graphics::Renderer *r);
	ScannerWidget(Graphics::Renderer *r, Serializer::Reader &rd);
	virtual ~ScannerWidget();
	void GetSizeRequested(float size[2]);
	void ToggleMode();
	void Draw();
	virtual void Update();

	void TimeStepUpdate(float step);

	void Save(Serializer::Writer &wr);

private:
	void InitObject();

	void DrawBlobs(bool below);
	void GenerateBaseGeometry();
	void GenerateRingsAndSpokes();
	void DrawRingsAndSpokes(bool blend);

	sigc::connection m_toggleScanModeConnection;

	struct Contact {
		Object::Type type;
		vector3d pos;
		bool isSpecial;
	};
	std::list<Contact> m_contacts;

	enum ScannerMode { SCANNER_MODE_AUTO, SCANNER_MODE_MANUAL };
	ScannerMode m_mode;

	float m_currentRange, m_manualRange, m_targetRange;
	float m_scale;

	float m_x;
	float m_y;

	float m_lastRange;

	std::vector<vector2f> m_circle;
	std::vector<vector2f> m_spokes;
	std::vector<vector2f> m_vts;
	std::vector<vector3f> m_edgeVts;
	std::vector<Color> m_edgeCols;

	Graphics::Renderer *m_renderer;
	Graphics::RenderState *m_renderState;
};

class UseEquipWidget: public IMultiFunc, public Gui::Fixed {
public:
	UseEquipWidget();
	virtual ~UseEquipWidget();
	void GetSizeRequested(float size[2]);
	virtual void Update() {}
private:
	void UpdateEquip();
	void UseRadarMapper();
	void UseHypercloudAnalyzer();
	enum { MAX_MISSILE_SLOTS = 8 };

	sigc::connection m_onPlayerEquipChangedCon;

	void FireMissile(int idx);
};

#endif /* _SHIPCPANELMULTIFUNCDISPLAYS_H */
