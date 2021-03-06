-- Copyright � 2013-14 Meteoric Games Ltd

local f = Faction:new('Japanese Star Empire')
	:description_short('The historical birthplace of humankind')
	:description('Centered on Earth, the Core Union is an alliance of "free" worlds that holds sway over most of humanities colonies')
	:homeworld(-9,9,0,0,0)
	:foundingDate(2800.0)
	:expansionRate(0.1)
	:military_name('Imperial Japanese Navy')
	:police_name('Police')
	:colour(0.7,0.6,0.1)

f:govtype_weight('EARTHDEMOC',    60)
f:govtype_weight('EARTHCOLONIAL', 40)

f:illegal_goods_probability('ANIMAL_MEAT',75)	-- fed/cis
f:illegal_goods_probability('LIVE_ANIMALS',75)	-- fed/cis
f:illegal_goods_probability('HAND_WEAPONS',100)	-- fed
f:illegal_goods_probability('BATTLE_WEAPONS',50)	--fed/cis
f:illegal_goods_probability('NERVE_GAS',100)--fed/cis
f:illegal_goods_probability('NARCOTICS',100)--fed
f:illegal_goods_probability('SLAVES',100)--fed/cis

f:add_to_factions('Japanese Star Empire')
