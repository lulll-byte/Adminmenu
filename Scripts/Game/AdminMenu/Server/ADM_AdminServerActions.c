modded class SCR_PlayerController
{
	protected bool m_bADMClientAuthenticated;
	protected bool m_bADMUnlimitedAmmo;
	protected ref array<int> m_aADMGodModePlayers = new array<int>();
	protected bool m_bADMGodModeActive;
	protected bool m_bADMFastRun;

	//---------------------------------------------------------------------------------------------
	// Public request methods (client-side, trigger server RPCs)

	void ADM_RequestLogin(string password)
	{
		Rpc(ADM_RpcAsk_Login, password);
	}

	void ADM_RequestPlayerList()
	{
		Rpc(ADM_RpcAsk_PlayerList);
	}

	void ADM_RequestModeration(int targetPlayerId, string targetIdentityId, int action, int banSeconds = 0)
	{
		Rpc(ADM_RpcAsk_Moderation, targetPlayerId, targetIdentityId, action, banSeconds);
	}

	void ADM_RequestTeleportSelf(vector position)
	{
		Rpc(ADM_RpcAsk_TeleportSelfToPosition, position);
	}

	void ADM_RequestTeleportSelfToPlayer(int targetPlayerId)
	{
		Rpc(ADM_RpcAsk_TeleportSelfToPlayer, targetPlayerId);
	}

	void ADM_RequestTeleportPlayerToMe(int targetPlayerId)
	{
		Rpc(ADM_RpcAsk_TeleportPlayerToMe, targetPlayerId);
	}

	void ADM_RequestSpawnItem(ResourceName prefab, bool intoInventory)
	{
		Rpc(ADM_RpcAsk_SpawnItem, prefab, intoInventory);
	}

	void ADM_RequestSpawnAI(ResourceName prefab, vector position)
	{
		Rpc(ADM_RpcAsk_SpawnAI, prefab, position);
	}

	void ADM_RequestSpawnAIAtMe()
	{
		Rpc(ADM_RpcAsk_SpawnAIAtMe);
	}

	void ADM_RequestSpawnAggroUnit()
	{
		Rpc(ADM_RpcAsk_SpawnAggroUnit);
	}

	void ADM_RequestSpawnNPCAtMe(ResourceName prefab)
	{
		Rpc(ADM_RpcAsk_SpawnNPCAtMe, prefab);
	}

	void ADM_RequestSpawnVehicle(ResourceName prefab)
	{
		Rpc(ADM_RpcAsk_SpawnVehicle, prefab);
	}

	void ADM_RequestToggleUnlimitedAmmo()
	{
		Rpc(ADM_RpcAsk_ToggleUnlimitedAmmo);
	}

	void ADM_RequestGodModePlayer(int targetPlayerId)
	{
		Rpc(ADM_RpcAsk_GodModePlayer, targetPlayerId);
	}

	void ADM_RequestHealPlayer(int targetPlayerId)
	{
		Rpc(ADM_RpcAsk_HealPlayer, targetPlayerId);
	}

	void ADM_RequestToggleFastRun()
	{
		Rpc(ADM_RpcAsk_ToggleFastRun);
	}

	//---------------------------------------------------------------------------------------------
	// Private helpers

	protected int ADM_GetRequesterPlayerId()
	{
		return GetPlayerId();
	}

	protected bool ADM_CanRunAdminAction()
	{
		return ADM_AdminAuthStore.IsAuthenticated(ADM_GetRequesterPlayerId());
	}

	protected IEntity ADM_GetPlayerEntity(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return null;

		return playerManager.GetPlayerControlledEntity(playerId);
	}

	protected void ADM_SetEntityPosition(IEntity entity, vector position)
	{
		if (!entity)
			return;

		entity.SetOrigin(position);
	}

	protected float ADM_GetSurfaceY(float x, float z, float offset = 0.5)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return offset;

		return world.GetSurfaceY(x, z) + offset;
	}

	// Recursively visits the entire entity subtree and fills every magazine to max.
	protected void ADM_FindAndRefillMagazines(IEntity entity)
	{
		if (!entity)
			return;

		BaseMagazineComponent mag = BaseMagazineComponent.Cast(entity.FindComponent(BaseMagazineComponent));
		if (mag)
		{
			int maxAmmo = mag.GetMaxAmmoCount();
			if (maxAmmo > 0)
				mag.SetAmmoCount(maxAmmo);
		}

		IEntity child = entity.GetChildren();
		while (child)
		{
			IEntity sibling = child.GetSibling();
			ADM_FindAndRefillMagazines(child);
			child = sibling;
		}
	}

	// Runs every 1 s while unlimited ammo is active; self-cancels if the entity is gone.
	protected void ADM_RefillAmmo()
	{
		if (!m_bADMUnlimitedAmmo)
			return;

		IEntity playerEntity = ADM_GetPlayerEntity(ADM_GetRequesterPlayerId());
		if (!playerEntity)
		{
			m_bADMUnlimitedAmmo = false;
			GetGame().GetCallqueue().Remove(ADM_RefillAmmo);
			return;
		}

		ADM_FindAndRefillMagazines(playerEntity);
	}

	protected void ADM_SpawnAIEntity(ResourceName prefab, vector spawnPos)
	{
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = spawnPos;

		IEntity spawned = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		SCR_AIGroup group = SCR_AIGroup.Cast(spawned);
		if (!group)
			return;

		if (!group.GetSpawnImmediately())
			group.SpawnUnits();

		ADM_WakeAIGroup(group);
		GetGame().GetCallqueue().CallLater(ADM_WakeAIGroup, 400, false, group);
	}

	protected void ADM_SpawnAggroAIEntity(ResourceName prefab, vector spawnPos, vector targetPos)
	{
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = spawnPos;

		IEntity spawned = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		SCR_AIGroup group = SCR_AIGroup.Cast(spawned);
		if (!group)
			return;

		if (!group.GetSpawnImmediately())
			group.SpawnUnits();

		// Search and Destroy waypoint at the target (player) position
		Resource wpRes = Resource.Load("{B3E7B8DC2BAB8ACC}Prefabs/AI/Waypoints/AIWaypoint_SearchAndDestroy.et");
		if (wpRes && wpRes.IsValid())
		{
			EntitySpawnParams wpParams = new EntitySpawnParams();
			wpParams.TransformMode = ETransformMode.WORLD;
			Math3D.MatrixIdentity4(wpParams.Transform);
			wpParams.Transform[3] = targetPos;
			IEntity wpEntity = GetGame().SpawnEntityPrefab(wpRes, GetGame().GetWorld(), wpParams);
			AIWaypoint wp = AIWaypoint.Cast(wpEntity);
			if (wp)
				group.AddWaypointToGroup(wp);
		}

		ADM_WakeAIGroup(group);
		GetGame().GetCallqueue().CallLater(ADM_WakeAIGroup, 400, false, group);
	}

	protected void ADM_WakeAIGroup(SCR_AIGroup group)
	{
		if (!group)
			return;

		group.ActivateAllMembers();
		group.PreventMaxLOD();

		array<AIAgent> agents = {};
		group.GetAgents(agents);
		int maxLod = AIAgent.GetMaxLOD();
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;
			agent.ActivateAI();
			if (agent.GetLOD() == maxLod)
				agent.SetLOD(maxLod - 1);
			agent.PreventMaxLOD();
		}
	}

	//---------------------------------------------------------------------------------------------
	// Server RPCs

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void ADM_RpcAsk_Login(string password)
	{
		int playerId = ADM_GetRequesterPlayerId();
		bool accepted = ADM_AdminAuthStore.IsAdminCandidate(playerId) && ADM_AdminAuthStore.ValidatePassword(password);
		ADM_AdminAuthStore.SetAuthenticated(playerId, accepted);
		Rpc(ADM_RpcDo_LoginResult, accepted);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void ADM_RpcAsk_PlayerList()
	{
		if (!ADM_CanRunAdminAction())
			return;

		array<int> playerIds = {};
		PlayerManager playerManager = GetGame().GetPlayerManager();
		playerManager.GetPlayers(playerIds);

		string packed;
		foreach (int playerId : playerIds)
		{
			string name = playerManager.GetPlayerName(playerId);
			string identityId = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
			packed += playerId.ToString() + "|" + name + "|" + identityId + "\n";
		}

		Rpc(ADM_RpcDo_PlayerList, packed);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void ADM_RpcAsk_Moderation(int targetPlayerId, string targetIdentityId, int action, int banSeconds)
	{
		if (!ADM_CanRunAdminAction())
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		if (action == ADM_AdminModerationAction.KICK)
		{
			playerManager.KickPlayer(targetPlayerId, PlayerManagerKickReason.KICK);
			return;
		}

		BanServiceApi banApi = GetGame().GetBackendApi().GetBanServiceApi();
		if (!banApi)
			return;

		string reason = "Admin menu action";
		if (targetIdentityId.IsEmpty() && targetPlayerId > 0)
			targetIdentityId = GetGame().GetBackendApi().GetPlayerIdentityId(targetPlayerId);

		if (targetIdentityId.IsEmpty())
			return;

		if (action == ADM_AdminModerationAction.BAN)
		{
			banApi.CreateBanIdentityId(null, targetIdentityId, reason, banSeconds);
			if (targetPlayerId > 0)
				playerManager.KickPlayer(targetPlayerId, PlayerManagerKickReason.BAN, banSeconds);
		}
		else if (action == ADM_AdminModerationAction.UNBAN)
		{
			array<string> identityIds = {};
			identityIds.Insert(targetIdentityId);
			banApi.RemoveBans(null, identityIds);
		}
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void ADM_RpcAsk_TeleportSelfToPosition(vector position)
	{
		if (!ADM_CanRunAdminAction())
			return;

		// Y = 0 means the client did not specify a height (named location, or default).
		// Snap to terrain surface + 0.5 m so the player lands on solid ground.
		if (position[1] == 0)
			position[1] = ADM_GetSurfaceY(position[0], position[2]);

		ADM_SetEntityPosition(ADM_GetPlayerEntity(ADM_GetRequesterPlayerId()), position);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void ADM_RpcAsk_TeleportSelfToPlayer(int targetPlayerId)
	{
		if (!ADM_CanRunAdminAction())
			return;

		IEntity target    = ADM_GetPlayerEntity(targetPlayerId);
		IEntity requester = ADM_GetPlayerEntity(ADM_GetRequesterPlayerId());
		if (target && requester)
			ADM_SetEntityPosition(requester, target.GetOrigin());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void ADM_RpcAsk_TeleportPlayerToMe(int targetPlayerId)
	{
		if (!ADM_CanRunAdminAction())
			return;

		IEntity requester = ADM_GetPlayerEntity(ADM_GetRequesterPlayerId());
		IEntity target    = ADM_GetPlayerEntity(targetPlayerId);
		if (target && requester)
			ADM_SetEntityPosition(target, requester.GetOrigin());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void ADM_RpcAsk_SpawnItem(ResourceName prefab, bool intoInventory)
	{
		if (!ADM_CanRunAdminAction())
			return;

		IEntity requester = ADM_GetPlayerEntity(ADM_GetRequesterPlayerId());
		if (!requester)
			return;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return;

		if (!intoInventory)
		{
			EntitySpawnParams params = new EntitySpawnParams();
			params.TransformMode = ETransformMode.WORLD;
			requester.GetWorldTransform(params.Transform);
			params.Transform[3] = requester.GetOrigin() + "0 0 1";
			GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
			return;
		}

		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(
			requester.FindComponent(InventoryStorageManagerComponent));
		if (inventory)
			inventory.TrySpawnPrefabToStorage(prefab);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void ADM_RpcAsk_SpawnAI(ResourceName prefab, vector position)
	{
		if (!ADM_CanRunAdminAction())
			return;

		// Always snap the group to terrain surface so it does not spawn underground.
		position[1] = ADM_GetSurfaceY(position[0], position[2], 0.1);
		ADM_SpawnAIEntity(prefab, position);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void ADM_RpcAsk_SpawnAIAtMe()
	{
		if (!ADM_CanRunAdminAction())
			return;

		IEntity requester = ADM_GetPlayerEntity(ADM_GetRequesterPlayerId());
		if (!requester)
			return;

		vector origin = requester.GetOrigin();

		vector spawnPos;
		spawnPos[0] = origin[0];
		spawnPos[2] = origin[2] + 5;
		spawnPos[1] = ADM_GetSurfaceY(spawnPos[0], spawnPos[2], 0.2);

		ADM_SpawnAIEntity(ADM_AdminMenuConstants.DEFAULT_AI_GROUP_PREFAB, spawnPos);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void ADM_RpcAsk_SpawnAggroUnit()
	{
		if (!ADM_CanRunAdminAction())
			return;

		IEntity requester = ADM_GetPlayerEntity(ADM_GetRequesterPlayerId());
		if (!requester)
			return;

		vector origin = requester.GetOrigin();

		vector spawnPos;
		spawnPos[0] = origin[0];
		spawnPos[2] = origin[2] + 6;
		spawnPos[1] = ADM_GetSurfaceY(spawnPos[0], spawnPos[2], 0.2);

		// USSR Fire Group (2-soldier pair) with Search and Destroy waypoint on player
		ADM_SpawnAggroAIEntity("{30ED11AA4F0D41E5}Prefabs/Groups/OPFOR/Group_USSR_FireGroup.et", spawnPos, origin);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void ADM_RpcAsk_SpawnNPCAtMe(ResourceName prefab)
	{
		if (!ADM_CanRunAdminAction())
			return;

		IEntity requester = ADM_GetPlayerEntity(ADM_GetRequesterPlayerId());
		if (!requester)
			return;

		vector origin = requester.GetOrigin();
		vector spawnPos;
		spawnPos[0] = origin[0] + 5;
		spawnPos[2] = origin[2];
		spawnPos[1] = ADM_GetSurfaceY(spawnPos[0], spawnPos[2], 0.2);

		ADM_SpawnAIEntity(prefab, spawnPos);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void ADM_RpcAsk_SpawnVehicle(ResourceName prefab)
	{
		if (!ADM_CanRunAdminAction())
			return;

		IEntity requester = ADM_GetPlayerEntity(ADM_GetRequesterPlayerId());
		if (!requester)
			return;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return;

		vector origin = requester.GetOrigin();
		vector spawnPos;
		spawnPos[0] = origin[0] + 6;
		spawnPos[2] = origin[2];
		spawnPos[1] = ADM_GetSurfaceY(spawnPos[0], spawnPos[2], 0.1);

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		requester.GetWorldTransform(params.Transform);
		params.Transform[3] = spawnPos;

		GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "Vehicle spawned at " + spawnPos, LogLevel.NORMAL);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void ADM_RpcAsk_ToggleUnlimitedAmmo()
	{
		if (!ADM_CanRunAdminAction())
			return;

		m_bADMUnlimitedAmmo = !m_bADMUnlimitedAmmo;

		if (m_bADMUnlimitedAmmo)
			GetGame().GetCallqueue().CallLater(ADM_RefillAmmo, 1000, true);
		else
			GetGame().GetCallqueue().Remove(ADM_RefillAmmo);

		Rpc(ADM_RpcDo_UnlimitedAmmoResult, m_bADMUnlimitedAmmo);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void ADM_RpcAsk_ToggleFastRun()
	{
		if (!ADM_CanRunAdminAction())
			return;

		m_bADMFastRun = !m_bADMFastRun;

		Rpc(ADM_RpcDo_FastRunResult, m_bADMFastRun);
		string fastRunState = "OFF";
		if (m_bADMFastRun) fastRunState = "ON";
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "Fast run " + fastRunState, LogLevel.NORMAL);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void ADM_RpcAsk_GodModePlayer(int targetPlayerId)
	{
		if (!ADM_CanRunAdminAction())
			return;

		int idx = m_aADMGodModePlayers.Find(targetPlayerId);
		bool enabling = idx < 0;

		if (enabling)
			m_aADMGodModePlayers.Insert(targetPlayerId);
		else
			m_aADMGodModePlayers.Remove(idx);

		if (!m_bADMGodModeActive && !m_aADMGodModePlayers.IsEmpty())
		{
			m_bADMGodModeActive = true;
			GetGame().GetCallqueue().CallLater(ADM_GodModeHeal, 500, true);
		}
		else if (m_aADMGodModePlayers.IsEmpty())
		{
			m_bADMGodModeActive = false;
			GetGame().GetCallqueue().Remove(ADM_GodModeHeal);
		}

		Rpc(ADM_RpcDo_GodModeResult, enabling);
		string godModeState = "OFF";
		if (enabling) godModeState = "ON";
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "God mode " + godModeState + " for playerId=" + targetPlayerId, LogLevel.NORMAL);
	}

	protected void ADM_GodModeHeal()
	{
		if (!m_aADMGodModePlayers || m_aADMGodModePlayers.IsEmpty())
		{
			m_bADMGodModeActive = false;
			GetGame().GetCallqueue().Remove(ADM_GodModeHeal);
			return;
		}

		for (int i = m_aADMGodModePlayers.Count() - 1; i >= 0; i--)
		{
			IEntity target = ADM_GetPlayerEntity(m_aADMGodModePlayers[i]);
			if (!target)
			{
				m_aADMGodModePlayers.Remove(i);
				continue;
			}
			SCR_CharacterDamageManagerComponent dm = SCR_CharacterDamageManagerComponent.Cast(
				target.FindComponent(SCR_CharacterDamageManagerComponent));
			if (dm)
				dm.FullHeal();
		}

		if (m_aADMGodModePlayers.IsEmpty())
		{
			m_bADMGodModeActive = false;
			GetGame().GetCallqueue().Remove(ADM_GodModeHeal);
		}
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void ADM_RpcAsk_HealPlayer(int targetPlayerId)
	{
		if (!ADM_CanRunAdminAction())
			return;

		IEntity target = ADM_GetPlayerEntity(targetPlayerId);
		if (!target)
			return;

		SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(
			target.FindComponent(SCR_CharacterDamageManagerComponent));
		if (damageManager)
			damageManager.FullHeal();
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "Heal for playerId=" + targetPlayerId, LogLevel.NORMAL);
	}

	//---------------------------------------------------------------------------------------------
	// Client RPCs

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void ADM_RpcDo_LoginResult(bool accepted)
	{
		m_bADMClientAuthenticated = accepted;
		ADM_AdminMenuOverlay.OnLoginResult(accepted);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void ADM_RpcDo_PlayerList(string packedPlayers)
	{
		ADM_AdminMenuOverlay.OnPlayerList(packedPlayers);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void ADM_RpcDo_UnlimitedAmmoResult(bool enabled)
	{
		ADM_AdminMenuOverlay.OnUnlimitedAmmoChanged(enabled);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void ADM_RpcDo_GodModeResult(bool enabled)
	{
		ADM_AdminMenuOverlay.OnGodModeChanged(enabled);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void ADM_RpcDo_FastRunResult(bool enabled)
	{
		m_bADMFastRun = enabled;
		if (m_bADMFastRun)
			GetGame().GetCallqueue().CallLater(ADM_FastRunClientTick, 50, true);
		else
			GetGame().GetCallqueue().Remove(ADM_FastRunClientTick);
		ADM_AdminMenuOverlay.OnFastRunChanged(enabled);
	}

	protected void ADM_FastRunClientTick()
	{
		if (!m_bADMFastRun)
		{
			GetGame().GetCallqueue().Remove(ADM_FastRunClientTick);
			return;
		}

		IEntity player = GetControlledEntity();
		if (!player)
			return;

		Physics phys = player.GetPhysics();
		if (!phys)
			return;

		vector vel = phys.GetVelocity();
		float hSpeedSq = vel[0] * vel[0] + vel[2] * vel[2];
		if (hSpeedSq < 0.25)
			return;

		phys.SetVelocity(Vector(vel[0] * 6, vel[1], vel[2] * 6));
	}
}
