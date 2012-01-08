-- Stats-gathering module. Initially, gathers kill statistics for the player.
-- Can (and should) be expanded in the future to gather other information.

-- The information gathered here is stored in the player's character sheet.
-- This is globally available to all Lua scripts. Retrieval methods should
-- be implemented as part of Characters.lua.

-- This is used for tracking which ships were damaged by the player, so that
-- we can award assists as well as kills. One day, assists might contribute to
-- the combat rating.
local PlayerDamagedShips = {}

-- We need this to translate "Right on, Commander" etc
local t = Translate:GetTranslator()


local onShipDestroyed = function (ship, attacker)
	if attacker:isa('Ship') and attacker:IsPlayer() then
		-- Increment player's kill count
		PersistentCharacters.player.killcount = PersistentCharacters.player.killcount + 1
		PlayerDamagedShips[ship]=nil
		if PersistentCharacters.player.killcount == 1 or (PersistentCharacters.player.killcount < 256 and PersistentCharacters.player.killcount % 16 == 0) or (PersistentCharacters.player.killcount % 256 == 0) then
			-- On the first kill, every 16th kill until 256, and every 256th
			-- kill thereafter
			UI.ImportantMessage(t('Right on, Commander!'),t('Elite Federation of Pilots'))
		end
	elseif PlayerDamagedShips[ship] then
		PersistentCharacters.player.assistcount = PersistentCharacters.player.assistcount + 1
	end
end

local onShipHit = function (ship, attacker)
	if attacker:isa('Ship') and attacker:IsPlayer() then
		PlayerDamagedShips[ship]=true
	end
end

EventQueue.onShipDestroyed:Connect(onShipDestroyed)
--Commented out pending issue #887
--EventQueue.onShipHit:Connect(onShipHit)
