define_orbital_station {
	model = 'tradestation',
	angular_velocity = 0.1,
	num_docking_ports = 20,
	parking_distance = 5000.0,
	parking_gap_size = 500.0,
	ship_launch_stage = 3,
	-- define groups of bays, in this case 1 group with 1 bay.
	-- params are = {minSize, maxSize, {list,of,bay,numbers}}
	bay_groups = {
		{30, 60, {1}},
		{0, 25, {2,3,4,5}},
		{30, 60, {6}},
		{0, 25, {7,8,9,10}},
		{30, 60, {11}},
		{0, 30, {12,13,14,15}},
		{30, 60, {16}},
		{0, 25, {17,18,19,20}},
	},
	dock_anim_stage_duration = { 300, 10.0, 5.0, 5.0 },
	undock_anim_stage_duration = { 5.0, 5.0, 10.0 },
}
