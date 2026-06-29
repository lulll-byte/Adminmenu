enum ADM_AdminMenuPage
{
	LOGIN,
	MAIN,
	PLAYERS,
	TELEPORT,
	TELEPORT_SELECT_PLAYER,
	SPAWN,
	SPAWN_ITEMS,
	SPAWN_ITEMS2,
	LOCATIONS,
	LOCATIONS_2,
	LOCATIONS_CHERNARUS,
	LOCATIONS_CHERNARUS_2,
	SPAWN_NPCS
}

enum ADM_AdminModerationAction
{
	KICK,
	BAN,
	UNBAN
}

enum ADM_TeleportSelectMode
{
	TO_PLAYER,
	PLAYER_TO_ME
}

class ADM_PlayerListEntry
{
	int m_iPlayerId;
	string m_sName;
	string m_sIdentityId;

	void ADM_PlayerListEntry(int playerId, string name, string identityId)
	{
		m_iPlayerId = playerId;
		m_sName = name;
		m_sIdentityId = identityId;
	}
}

class ADM_AdminMenuConstants
{
	static const string ADMIN_PASSWORD = "Reforgeradminaccess";
	static const string ACTION_TOGGLE = "ADM_ToggleAdminMenu";
	static const string ADMIN_MENU_CONTEXT = "ADM_AdminMenuContext";
	static const string LOG_PREFIX = "ADM AdminMenu: ";

	static const ResourceName DEFAULT_ITEM_PREFAB = "{00E36F41CA310E2A}Prefabs/Items/Medicine/SalineBag_01/SalineBag_US_01.et";
	static const ResourceName DEFAULT_AI_GROUP_PREFAB = "{657590C1EC9E27D3}Prefabs/Groups/OPFOR/Group_USSR_LightFireTeam.et";
	static const ResourceName DEFAULT_AI_UNIT_PREFAB  = "{657590C1EC9E27D3}Prefabs/Groups/OPFOR/Group_USSR_LightFireTeam.et";
	static const ResourceName AI_DEFEND_WAYPOINT      = "{FAD1D789EE291964}Prefabs/AI/Waypoints/AIWaypoint_Defend_Large.et";
}

// All ResourceName strings have been verified against the Epoch4 mod asset registry
// and the Arma Reforger base game Prefabs structure.
class ADM_ItemPrefabs
{
	// -- Rifles ----------------------------------------------------------------
	static const ResourceName M16A2         = "{3E413771E1834D2F}Prefabs/Weapons/Rifles/M16/Rifle_M16A2.et";
	static const ResourceName AK74          = "{FA5C25BF66A53DCF}Prefabs/Weapons/Rifles/AK74/Rifle_AK74.et";
	static const ResourceName AKS74U        = "{BFEA719491610A45}Prefabs/Weapons/Rifles/AKS74U/Rifle_AKS74U.et";
	static const ResourceName SVD           = "{3EB02CDAD5F23C82}Prefabs/Weapons/Rifles/SVD/Rifle_SVD.et";

	// -- Pistols ---------------------------------------------------------------
	static const ResourceName M9            = "{1353C6EAD1DCFE43}Prefabs/Weapons/Handguns/M9/Handgun_M9.et";
	static const ResourceName PM            = "{C0F7DD85A86B2900}Prefabs/Weapons/Handguns/PM/Handgun_PM.et";

	// -- Launchers -------------------------------------------------------------
	static const ResourceName RPG7          = "{7A82FE978603F137}Prefabs/Weapons/Launchers/RPG7/Launcher_RPG7.et";
	static const ResourceName M72LAW        = "{9C5C20FB0E01E64F}Prefabs/Weapons/Launchers/M72/Launcher_M72A3.et";

	// -- Machine guns ----------------------------------------------------------
	static const ResourceName PKM           = "{A89BC9D55FFB4CD8}Prefabs/Weapons/MachineGuns/PKM/MG_PKM.et";
	static const ResourceName M249          = "{D2B48DEBEF38D7D7}Prefabs/Weapons/MachineGuns/M249/MG_M249.et";

	// -- Medical ---------------------------------------------------------------
	static const ResourceName MedkitUS      = "{AE578EEA4244D41F}Prefabs/Items/Equipment/Kits/MedicalKit_01/MedicalKit_01_US.et";
	static const ResourceName SalineBagUS   = "{00E36F41CA310E2A}Prefabs/Items/Medicine/SalineBag_01/SalineBag_US_01.et";
	static const ResourceName Morphine      = "{0D9A5DCF89AE7AA9}Prefabs/Items/Medicine/MorphineInjection_01/MorphineInjection_01.et";
	static const ResourceName FieldDressing = "{A81F501D3EF6F38E}Prefabs/Items/Medicine/FieldDressing_01/FieldDressing_US_01.et";
	static const ResourceName TourniquetUS  = "{D70216B1B2889129}Prefabs/Items/Medicine/Tourniquet_01/Tourniquet_US_01.et";

	// -- Equipment -------------------------------------------------------------
	static const ResourceName BinocsUS      = "{0CF54B9A85D8E0D4}Prefabs/Items/Equipment/Binoculars/Binoculars_M22/Binoculars_M22.et";
	static const ResourceName RadioUS       = "{73950FBA2D7DB5C5}Prefabs/Items/Equipment/Radios/Radio_ANPRC68.et";
	static const ResourceName WaterBottle   = "{2DA40953CC5C6D88}Prefabs/FoodDrink/WaterBottler.et";
	static const ResourceName Jerrycan      = "{12D5AD21E383B768}Prefabs/Items/Fuel/Jerrycan_01/Jerrycan_01_item.et";

	// -- Vehicles (wheeled) ----------------------------------------------------
	static const ResourceName HMMWV         = "{5674FAEB9AB7BDD0}Prefabs/Vehicles/Wheeled/M998/M998.et";
	static const ResourceName UAZ469        = "{259EE7B78C51B624}Prefabs/Vehicles/Wheeled/UAZ469/UAZ469.et";
	static const ResourceName M923A1        = "{81FDAD5EB644CC3D}Prefabs/Vehicles/Wheeled/M923A1/M923A1_transport_covered.et";
	static const ResourceName Ural4320      = "{16C1F16C9B053801}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320_transport.et";

	// -- Vehicles (air) --------------------------------------------------------
	static const ResourceName Huey          = "{70BAEEFC2D3FEE64}Prefabs/Vehicles/Helicopters/UH1H/UH1H.et";
	static const ResourceName Mi8MT         = "{DF5CCB7C0FF049F4}Prefabs/Vehicles/Helicopters/Mi8MT/Mi8MT_unarmed_transport.et";
}
