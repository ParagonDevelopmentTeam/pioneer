-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of CC-BY-SA 3.0. See licenses/CC-BY-SA-3.0.txt

--Ships not available for purchase (ambient ships)
define_ship {
	name='Omni Explorer',
	model='omni_explorer',
	forward_thrust = 20000e5,
	reverse_thrust = 20000e5,
	up_thrust = 120000e5,
	down_thrust = 120000e5,
	left_thrust = 120000e5,
	right_thrust = 120000e5,
	angular_thrust = 3000000e5,
	max_cargo = 2000,
	max_laser = 2,
	max_missile = 0,
	max_cargoscoop = 0,
	min_crew = 6,
	max_crew = 20,
	capacity = 2000,
	hull_mass = 10000,
	fuel_tank_mass = 6000,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 55123e3,
	price = 120000000,
	hyperdrive_class = 10,
}