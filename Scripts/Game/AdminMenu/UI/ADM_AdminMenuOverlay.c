class ADM_AdminMenuButtonHandler : ScriptedWidgetEventHandler
{
	protected ADM_AdminMenuOverlay m_Menu;
	protected int m_iIndex;

	void ADM_AdminMenuButtonHandler(ADM_AdminMenuOverlay menu, int index)
	{
		m_Menu = menu;
		m_iIndex = index;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Menu)
			m_Menu.ActivateButton(m_iIndex);

		return true;
	}
}

class ADM_AdminMenuOverlay : ScriptedWidgetEventHandler
{
	protected static ref ADM_AdminMenuOverlay s_Instance;

	protected Widget m_wRoot;
	protected TextWidget m_wTitle;
	protected TextWidget m_wStatus;
	protected EditBoxWidget m_wInputA;
	protected EditBoxWidget m_wInputB;
	protected EditBoxWidget m_wInputC;
	protected ref array<ButtonWidget> m_aButtons = {};
	protected ref array<ref ADM_AdminMenuButtonHandler> m_aButtonHandlers = {};
	protected ref array<ref ADM_PlayerListEntry> m_aPlayers = {};
	protected ADM_AdminMenuPage m_ePage = ADM_AdminMenuPage.LOGIN;
	protected int m_iSelection;
	protected int m_iSelectedPlayerId;
	protected string m_sSelectedIdentityId;
	protected ADM_TeleportSelectMode m_eTeleportSelectMode;
	protected bool m_bClientUnlimitedAmmo;
	protected bool m_bClientGodMode;
	protected bool m_bClientFastRun;
	protected ref map<int, bool> m_mGodModeStates = new map<int, bool>();
	protected int m_iFocusCursor;

	//---------------------------------------------------------------------------------------------
	// Static interface

	static void Toggle()
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "Toggle() called", LogLevel.NORMAL);

		if (s_Instance && s_Instance.IsOpen())
		{
			s_Instance.Close();
			return;
		}

		if (!s_Instance)
			s_Instance = new ADM_AdminMenuOverlay();

		s_Instance.Open();
	}

	static void PollNavKeys()
	{
		if (!s_Instance || !s_Instance.IsOpen())
			return;
		s_Instance.DoPollNavKeys();
	}

	protected bool m_bNavUpWasDown;
	protected bool m_bNavDownWasDown;
	protected bool m_bNavSelectWasDown;

	protected void DoPollNavKeys()
	{
		bool upNow = Debug.KeyState(KeyCode.KC_UP);
		if (upNow && !m_bNavUpWasDown) OnMenuUp(0, EActionTrigger.DOWN);
		m_bNavUpWasDown = upNow;

		bool downNow = Debug.KeyState(KeyCode.KC_DOWN);
		if (downNow && !m_bNavDownWasDown) OnMenuDown(0, EActionTrigger.DOWN);
		m_bNavDownWasDown = downNow;

		bool selNow = Debug.KeyState(KeyCode.KC_RETURN);
		if (selNow && !m_bNavSelectWasDown) OnMenuSelect(0, EActionTrigger.DOWN);
		m_bNavSelectWasDown = selNow;
	}

	static void OnLoginResult(bool accepted)
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "OnLoginResult accepted=" + accepted, LogLevel.NORMAL);
		if (s_Instance)
			s_Instance.HandleLoginResult(accepted);
	}

	static void OnPlayerList(string packedPlayers)
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "OnPlayerList len=" + packedPlayers.Length(), LogLevel.NORMAL);
		if (s_Instance)
			s_Instance.HandlePlayerList(packedPlayers);
	}

	static void OnUnlimitedAmmoChanged(bool enabled)
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "OnUnlimitedAmmoChanged enabled=" + enabled, LogLevel.NORMAL);
		if (s_Instance)
			s_Instance.HandleUnlimitedAmmoChanged(enabled);
	}

	static void OnGodModeChanged(bool enabled)
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "OnGodModeChanged enabled=" + enabled, LogLevel.NORMAL);
		if (s_Instance)
			s_Instance.HandleGodModeChanged(enabled);
	}

	static void OnFastRunChanged(bool enabled)
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "OnFastRunChanged enabled=" + enabled, LogLevel.NORMAL);
		if (s_Instance)
			s_Instance.HandleFastRunChanged(enabled);
	}

	//---------------------------------------------------------------------------------------------
	// Open / Close

	bool IsOpen()
	{
		return m_wRoot != null;
	}

	void Open()
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "=== Open() START ===", LogLevel.NORMAL);

		if (m_wRoot)
		{
			Print(ADM_AdminMenuConstants.LOG_PREFIX + "Open() already open", LogLevel.NORMAL);
			return;
		}

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
		{
			Print(ADM_AdminMenuConstants.LOG_PREFIX + "Open() FAIL no workspace", LogLevel.ERROR);
			return;
		}

		m_wRoot = workspace.CreateWidgets("{6A44E2C900000004}UI/layouts/AdminMenu/AdminMenu.layout");
		if (!m_wRoot)
		{
			Print(ADM_AdminMenuConstants.LOG_PREFIX + "Open() FAIL CreateWidgets returned null", LogLevel.ERROR);
			return;
		}

		Print(ADM_AdminMenuConstants.LOG_PREFIX + "Open() layout loaded", LogLevel.NORMAL);

		m_wRoot.AddHandler(this);
		m_wRoot.SetVisible(true);
		m_wRoot.SetZOrder(1000);

		m_wTitle  = TextWidget.Cast(m_wRoot.FindAnyWidget("Title"));
		m_wStatus = TextWidget.Cast(m_wRoot.FindAnyWidget("Status"));
		m_wInputA = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("InputA"));
		m_wInputB = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("InputB"));
		m_wInputC = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("InputC"));
		if (m_wInputA)
			m_wInputA.AddHandler(this);

		Widget dimBg = m_wRoot.FindAnyWidget("DimBackground");
		if (dimBg)
		{
			dimBg.SetVisible(true);
			dimBg.SetZOrder(0);
		}

		Widget panel = m_wRoot.FindAnyWidget("Panel");
		if (panel)
		{
			panel.SetVisible(true);
			panel.SetZOrder(100);
		}

		Widget panelBg = m_wRoot.FindAnyWidget("PanelBg");
		if (panelBg)
		{
			panelBg.SetVisible(true);
			panelBg.SetZOrder(0);
		}

		ADM_ApplyGeometry(panel, panelBg);

		if (m_wTitle)
		{
			m_wTitle.SetVisible(true);
			m_wTitle.SetZOrder(200);
			m_wTitle.SetIsColorInherited(false);
		}
		if (m_wStatus)
		{
			m_wStatus.SetVisible(true);
			m_wStatus.SetZOrder(200);
			m_wStatus.SetIsColorInherited(false);
		}
		if (m_wInputA) { m_wInputA.SetVisible(true); m_wInputA.SetZOrder(200); }
		if (m_wInputB) { m_wInputB.SetVisible(true); m_wInputB.SetZOrder(200); }
		if (m_wInputC) { m_wInputC.SetVisible(true); m_wInputC.SetZOrder(200); }

		Print(ADM_AdminMenuConstants.LOG_PREFIX + "Open() widgets visible: panel=" + (panel != null) + " title=" + (m_wTitle != null) + " status=" + (m_wStatus != null), LogLevel.NORMAL);

		InputManager im = GetGame().GetInputManager();
		m_iFocusCursor = 0;

		if (im)
		{
			im.AddActionListener("MenuUp",             EActionTrigger.DOWN, OnMenuUp);
			im.AddActionListener("MenuDown",           EActionTrigger.DOWN, OnMenuDown);
			im.AddActionListener("MenuSelect",         EActionTrigger.DOWN, OnMenuSelect);
			im.AddActionListener("ADM_AdminNextField", EActionTrigger.DOWN, OnNextField);
			Print(ADM_AdminMenuConstants.LOG_PREFIX + "Open() input listeners registered", LogLevel.NORMAL);
		}
		else
		{
			Print(ADM_AdminMenuConstants.LOG_PREFIX + "Open() WARNING no InputManager", LogLevel.WARNING);
		}

		ShowLogin();

		workspace.AddModal(m_wRoot, m_wInputA);

		if (m_wInputA)
			workspace.SetFocusedWidget(m_wInputA, true);

		if (im)
			im.SetCursorPosition(workspace.GetWidth() / 2, workspace.GetHeight() / 2);

		WidgetManager.SetCursor(0);

		m_wRoot.Update();

		Print(ADM_AdminMenuConstants.LOG_PREFIX + "=== Open() DONE ===", LogLevel.NORMAL);
	}

	void Close()
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "Close() called", LogLevel.NORMAL);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace && m_wRoot)
			workspace.RemoveModal(m_wRoot);

		InputManager im = GetGame().GetInputManager();
		if (im)
		{
			im.RemoveActionListener("MenuUp",             EActionTrigger.DOWN, OnMenuUp);
			im.RemoveActionListener("MenuDown",           EActionTrigger.DOWN, OnMenuDown);
			im.RemoveActionListener("MenuSelect",         EActionTrigger.DOWN, OnMenuSelect);
			im.RemoveActionListener("ADM_AdminNextField", EActionTrigger.DOWN, OnNextField);
			Print(ADM_AdminMenuConstants.LOG_PREFIX + "Close() input listeners removed", LogLevel.NORMAL);
		}

		if (m_wRoot)
			m_wRoot.RemoveHandler(this);

		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();

		m_wRoot = null;
		m_aButtons.Clear();
		m_aButtonHandlers.Clear();

		Print(ADM_AdminMenuConstants.LOG_PREFIX + "Close() done", LogLevel.NORMAL);
	}

	//---------------------------------------------------------------------------------------------
	// Button helpers

	void ActivateButton(int index)
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ActivateButton index=" + index + " page=" + m_ePage, LogLevel.NORMAL);
		m_iSelection = index;
		RefreshSelection();
		ActivateSelection();
	}

	protected SCR_PlayerController GetLocalController()
	{
		return SCR_PlayerController.Cast(GetGame().GetPlayerController());
	}

	protected void SetTitle(string text)
	{
		if (m_wTitle)
			m_wTitle.SetText(text);
	}

	protected void SetStatus(string text)
	{
		if (m_wStatus)
			m_wStatus.SetText(text);
	}

	protected void SetInputs(string a = "", string b = "", string c = "")
	{
		if (m_wInputA) m_wInputA.SetText(a);
		if (m_wInputB) m_wInputB.SetText(b);
		if (m_wInputC) m_wInputC.SetText(c);
	}

	protected void AddButton(string widgetName, string label)
	{
		if (!m_wRoot)
		{
			Print(ADM_AdminMenuConstants.LOG_PREFIX + "AddButton FAIL no root for " + widgetName, LogLevel.ERROR);
			return;
		}

		ButtonWidget button = ButtonWidget.Cast(m_wRoot.FindAnyWidget(widgetName));
		if (!button)
		{
			Print(ADM_AdminMenuConstants.LOG_PREFIX + "AddButton FAIL widget not found " + widgetName, LogLevel.WARNING);
			return;
		}

		TextWidget lbl = TextWidget.Cast(button.FindAnyWidget("Label"));
		if (lbl)
			lbl.SetText(label);
		else
			Print(ADM_AdminMenuConstants.LOG_PREFIX + "AddButton no Label child in " + widgetName, LogLevel.WARNING);

		button.SetVisible(true);
		button.SetZOrder(1005);

		ADM_AdminMenuButtonHandler handler = new ADM_AdminMenuButtonHandler(this, m_aButtons.Count());
		button.AddHandler(handler);
		m_aButtonHandlers.Insert(handler);
		m_aButtons.Insert(button);

		Print(ADM_AdminMenuConstants.LOG_PREFIX + "AddButton [" + (m_aButtons.Count() - 1) + "] " + widgetName + " = " + label, LogLevel.NORMAL);
	}

	protected void ClearButtons()
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ClearButtons count=" + m_aButtons.Count(), LogLevel.NORMAL);

		for (int i = 0; i < m_aButtons.Count() && i < m_aButtonHandlers.Count(); i++)
		{
			if (m_aButtons[i] && m_aButtonHandlers[i])
				m_aButtons[i].RemoveHandler(m_aButtonHandlers[i]);
		}

		m_aButtons.Clear();
		m_aButtonHandlers.Clear();

		if (!m_wRoot) return;

		for (int i = 0; i < 12; i++)
		{
			ButtonWidget b = ButtonWidget.Cast(m_wRoot.FindAnyWidget("Button" + i));
			if (b)
				b.SetVisible(false);
		}
	}

	protected void RefreshSelection()
	{
		// On the PLAYERS page, auto-track which player the cursor is hovering on
		// so actions fire without needing a separate Enter press on the player row.
		if (m_ePage == ADM_AdminMenuPage.PLAYERS && m_iSelection < m_aPlayers.Count())
		{
			int hoverId = m_aPlayers[m_iSelection].m_iPlayerId;
			if (hoverId != m_iSelectedPlayerId)
			{
				m_iSelectedPlayerId   = hoverId;
				m_sSelectedIdentityId = m_aPlayers[m_iSelection].m_sIdentityId;
				if (m_mGodModeStates.Contains(m_iSelectedPlayerId))
					m_bClientGodMode = m_mGodModeStates.Get(m_iSelectedPlayerId);
				else
					m_bClientGodMode = false;
				RefreshActionButtonLabels();
			}
		}

		for (int i = 0; i < m_aButtons.Count(); i++)
		{
			ButtonWidget b = m_aButtons[i];
			if (!b) continue;

			if (i == m_iSelection)
				b.SetColorInt(0xFF3A6EA5);
			else
				b.SetColorInt(0xFF202020);
		}
	}

	//---------------------------------------------------------------------------------------------
	// Page: Login

	protected void ShowLogin()
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ShowLogin() auto-submitting", LogLevel.NORMAL);
		m_ePage = ADM_AdminMenuPage.LOGIN;
		SetTitle("Admin Menu");
		SetStatus("Connecting...");
		SCR_PlayerController controller = GetLocalController();
		if (controller)
			controller.ADM_RequestLogin("");
	}

	//---------------------------------------------------------------------------------------------
	// Page: Main

	protected void ShowMain()
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ShowMain()", LogLevel.NORMAL);
		m_ePage = ADM_AdminMenuPage.MAIN;
		m_iSelection = 0;
		ClearButtons();
		SetTitle("Admin Menu");
		SetStatus("Arrow keys + Enter to navigate. Mouse clicks work (cursor may be invisible).");
		SetInputs("", "", "");
		AddButton("Button0", "Player management");
		AddButton("Button1", "Teleport tools");
		AddButton("Button2", "Spawning tools");
		AddButton("Button3", "Kill all nearby NPCs");
		AddButton("Button4", "Close");
		RefreshSelection();
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ShowMain() added " + m_aButtons.Count() + " buttons", LogLevel.NORMAL);
	}

	//---------------------------------------------------------------------------------------------
	// Page: Players

	protected void ShowPlayers()
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ShowPlayers()", LogLevel.NORMAL);
		m_ePage = ADM_AdminMenuPage.PLAYERS;
		m_iSelection = 0;
		ClearButtons();
		SetTitle("Player Management");
		SetStatus("Refreshing player list...");
		SetInputs("", "", "");

		SCR_PlayerController controller = GetLocalController();
		if (controller)
			controller.ADM_RequestPlayerList();
		else
			Print(ADM_AdminMenuConstants.LOG_PREFIX + "ShowPlayers() no local controller", LogLevel.WARNING);
	}

	//---------------------------------------------------------------------------------------------
	// Page: Teleport

	protected void ShowTeleport()
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ShowTeleport()", LogLevel.NORMAL);
		m_ePage = ADM_AdminMenuPage.TELEPORT;
		m_iSelection = 0;
		ClearButtons();
		SetTitle("Teleport Tools");
		SetStatus("Tip: open the map, hover cursor over terrain, then click 'Map cursor'.");
		SetInputs("", "", "");
		AddButton("Button0", "Everon...");
		AddButton("Button1", "Chernarus...");
		AddButton("Button2", "Teleport TO player...");
		AddButton("Button3", "Teleport player TO me...");
		AddButton("Button4", "Teleport to map cursor");
		AddButton("Button5", "Back");
		RefreshSelection();
	}

	//---------------------------------------------------------------------------------------------
	// Page: Teleport player selector

	protected void ShowTeleportSelectPlayer(ADM_TeleportSelectMode mode)
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ShowTeleportSelectPlayer() mode=" + mode, LogLevel.NORMAL);
		m_eTeleportSelectMode = mode;
		m_ePage = ADM_AdminMenuPage.TELEPORT_SELECT_PLAYER;
		m_iSelection = 0;
		ClearButtons();
		SetTitle("Select Player");
		SetStatus("Refreshing player list...");
		SetInputs("", "", "");

		SCR_PlayerController controller = GetLocalController();
		if (controller)
			controller.ADM_RequestPlayerList();
	}

	//---------------------------------------------------------------------------------------------
	// Page: Locations towns

	protected void ShowLocations()
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ShowLocations()", LogLevel.NORMAL);
		m_ePage = ADM_AdminMenuPage.LOCATIONS;
		m_iSelection = 0;
		ClearButtons();
		SetTitle("Everon - Towns (1/2)");
		SetStatus("Click a town to teleport. Height auto-computed from terrain.");
		SetInputs("", "", "");
		AddButton("Button0",  "Morton");
		AddButton("Button1",  "Regina");
		AddButton("Button2",  "Durras");
		AddButton("Button3",  "Saint Pierre Harbour");
		AddButton("Button4",  "Montignac");
		AddButton("Button5",  "Le Moule");
		AddButton("Button6",  "Almara");
		AddButton("Button7",  "Chapoi");
		AddButton("Button8",  "Houdan");
		AddButton("Button9",  "Terria");
		AddButton("Button10", "More towns & Airfields...");
		AddButton("Button11", "Back");
		RefreshSelection();
	}

	//---------------------------------------------------------------------------------------------
	// Page: Locations page 2 (more towns + airfields)

	protected void ShowLocations2()
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ShowLocations2()", LogLevel.NORMAL);
		m_ePage = ADM_AdminMenuPage.LOCATIONS_2;
		m_iSelection = 0;
		ClearButtons();
		SetTitle("Everon - Towns & Airfields (2/2)");
		SetStatus("Click a location to teleport. Height auto-computed from terrain.");
		SetInputs("", "", "");
		AddButton("Button0", "Briars");
		AddButton("Button1", "Tyrone");
		AddButton("Button2", "Vaux");
		AddButton("Button3", "Saint Philippe");
		AddButton("Button4", "[Airport] Le Moule Airfield");
		AddButton("Button5", "[Airport] Tyrone Airfield");
		AddButton("Button10", "Back to towns (1/2)...");
		AddButton("Button11", "Back");
		RefreshSelection();
	}

	//---------------------------------------------------------------------------------------------
	// Page: Locations Chernarus page 1

	protected void ShowLocationsChernarus()
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ShowLocationsChernarus()", LogLevel.NORMAL);
		m_ePage = ADM_AdminMenuPage.LOCATIONS_CHERNARUS;
		m_iSelection = 0;
		ClearButtons();
		SetTitle("Chernarus - Major Cities (1/2)");
		SetStatus("Click a location to teleport. Height auto-computed from terrain.");
		SetInputs("", "", "");
		AddButton("Button0",  "Chernogorsk");
		AddButton("Button1",  "Elektrozavodsk");
		AddButton("Button2",  "Berezino");
		AddButton("Button3",  "Zelenogorsk");
		AddButton("Button4",  "Krasnostav");
		AddButton("Button5",  "Solnichniy");
		AddButton("Button6",  "Kamyshovo");
		AddButton("Button7",  "Mogilevka");
		AddButton("Button8",  "Stary Sobor");
		AddButton("Button9",  "Vybor");
		AddButton("Button10", "More towns & Airfields...");
		AddButton("Button11", "Back");
		RefreshSelection();
	}

	//---------------------------------------------------------------------------------------------
	// Page: Locations Chernarus page 2

	protected void ShowLocationsChernarus2()
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ShowLocationsChernarus2()", LogLevel.NORMAL);
		m_ePage = ADM_AdminMenuPage.LOCATIONS_CHERNARUS_2;
		m_iSelection = 0;
		ClearButtons();
		SetTitle("Chernarus - Towns & Airfields (2/2)");
		SetStatus("Click a location to teleport. Height auto-computed from terrain.");
		SetInputs("", "", "");
		AddButton("Button0", "Novy Sobor");
		AddButton("Button1", "Kabanino");
		AddButton("Button2", "Rogovo");
		AddButton("Button3", "Pavlovo");
		AddButton("Button4", "Svetlojarsk");
		AddButton("Button5", "[Airfield] NWAF");
		AddButton("Button6", "[Airfield] Balota");
		AddButton("Button7", "[Airfield] Krasnostav Airfield");
		AddButton("Button10", "Back to cities (1/2)...");  // array idx 8
		AddButton("Button11", "Back");                      // array idx 9
		RefreshSelection();
	}

	//---------------------------------------------------------------------------------------------
	// Page: Spawn

	protected void ShowSpawn()
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ShowSpawn()", LogLevel.NORMAL);
		m_ePage = ADM_AdminMenuPage.SPAWN;
		m_iSelection = 0;
		ClearButtons();

		string defaultX = "0";
		string defaultZ = "0";
		SCR_PlayerController controller = GetLocalController();
		if (controller)
		{
			IEntity playerEntity = controller.GetControlledEntity();
			if (playerEntity)
			{
				vector pos = playerEntity.GetOrigin();
				defaultX = "" + pos[0];
				defaultZ = "" + pos[2];
			}
		}

		SetTitle("Spawning Tools");
		SetStatus("InputA: item prefab path.  InputB/C: X/Z for AI group at coords.");
		SetInputs(ADM_AdminMenuConstants.DEFAULT_ITEM_PREFAB, defaultX, defaultZ);
		AddButton("Button0",  "Spawn item at feet   [InputA]");
		AddButton("Button1",  "Spawn item to inventory   [InputA]");
		AddButton("Button2",  "Browse preset items...");
		AddButton("Button3",  "Browse NPCs...");
		AddButton("Button4",  "Spawn AI group at my position");
		AddButton("Button5",  "Spawn AI group at XYZ   [InputB/C]");
		AddButton("Button6",  "Spawn aggro USSR pair at me");
		AddButton("Button7",  "Spawn HMMWV next to me");
		AddButton("Button8",  "Spawn UAZ-469 next to me");
		AddButton("Button9",  "Back");
		RefreshSelection();
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ShowSpawn() added " + m_aButtons.Count() + " buttons", LogLevel.NORMAL);
	}

	//---------------------------------------------------------------------------------------------
	// Page: Preset items weapons and medical

	protected void ShowSpawnItems()
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ShowSpawnItems()", LogLevel.NORMAL);
		m_ePage = ADM_AdminMenuPage.SPAWN_ITEMS;
		m_iSelection = 0;
		ClearButtons();
		SetTitle("Preset Items - Weapons and Medical");
		SetStatus("Select an item to load into InputA, then go back and spawn.");
		SetInputs("", "", "");
		AddButton("Button0",  "M16A2  (US rifle)");
		AddButton("Button1",  "AK-74  (Soviet rifle)");
		AddButton("Button2",  "AKS74U  (compact AK)");
		AddButton("Button3",  "SVD  (sniper rifle)");
		AddButton("Button4",  "M9 Pistol  (US)");
		AddButton("Button5",  "PM Makarov Pistol  (Soviet)");
		AddButton("Button6",  "RPG-7  (launcher)");
		AddButton("Button7",  "PKM  (machine gun)");
		AddButton("Button8",  "Medical Kit  (US)");
		AddButton("Button9",  "Saline Bag  (US)");
		AddButton("Button10", "Vehicles and equipment...");
		AddButton("Button11", "Back");
		RefreshSelection();
	}

	//---------------------------------------------------------------------------------------------
	// Page: Preset items vehicles and equipment

	protected void ShowSpawnItems2()
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ShowSpawnItems2()", LogLevel.NORMAL);
		m_ePage = ADM_AdminMenuPage.SPAWN_ITEMS2;
		m_iSelection = 0;
		ClearButtons();
		SetTitle("Preset Items - Vehicles and Equipment");
		SetStatus("Select an item to load into InputA, then go back and spawn.");
		SetInputs("", "", "");
		AddButton("Button0",  "M998 HMMWV  (US jeep)");
		AddButton("Button1",  "UAZ-469  (Soviet jeep)");
		AddButton("Button2",  "UH-1H Huey  (US helicopter)");
		AddButton("Button3",  "Mi-8MT  (Soviet helicopter)");
		AddButton("Button4",  "M923A1  (US transport truck)");
		AddButton("Button5",  "Ural-4320  (Soviet truck)");
		AddButton("Button6",  "Binoculars M22  (US)");
		AddButton("Button7",  "Radio AN/PRC-68  (US)");
		AddButton("Button8",  "Field Dressing  (US)");
		AddButton("Button9",  "Tourniquet  (US)");
		AddButton("Button10", "Weapons and medical...");
		AddButton("Button11", "Back");
		RefreshSelection();
	}

	//---------------------------------------------------------------------------------------------
	// Page: Spawn NPCs

	protected void ShowSpawnNPCs()
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ShowSpawnNPCs()", LogLevel.NORMAL);
		m_ePage = ADM_AdminMenuPage.SPAWN_NPCS;
		m_iSelection = 0;
		ClearButtons();
		SetTitle("Spawn NPCs");
		SetStatus("Selects and spawns immediately next to you. OPFOR = hostile to US players.");
		SetInputs("", "", "");
		AddButton("Button0", "[OPFOR] USSR Fire Pair  (2 soldiers, hostile)");
		AddButton("Button1", "[OPFOR] USSR Light Fire Team  (4 soldiers, hostile)");
		AddButton("Button2", "[OPFOR] USSR Rifle Squad  (8+ soldiers, hostile)");
		AddButton("Button3", "[INDFOR] FIA Light Team");
		AddButton("Button4", "[INDFOR] FIA Machine Gun Team");
		AddButton("Button5", "[INDFOR] FIA Recon Team");
		AddButton("Button6", "[EP4] Zombie Pack");
		AddButton("Button7", "Back");
		RefreshSelection();
	}

	protected void SelectItemAndReturn(ResourceName prefab, string name)
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "SelectItemAndReturn " + name, LogLevel.NORMAL);
		ShowSpawn();
		if (m_wInputA)
			m_wInputA.SetText(prefab);
		SetStatus("Selected: " + name + "  -  Spawn at feet or Spawn to inventory.");
	}

	//---------------------------------------------------------------------------------------------
	// Callbacks from server RPCs

	protected void HandleLoginResult(bool accepted)
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "HandleLoginResult accepted=" + accepted, LogLevel.NORMAL);
		if (accepted)
			ShowMain();
		else
			SetStatus("Login failed - wrong password.");
	}

	protected void HandlePlayerList(string packedPlayers)
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "HandlePlayerList page=" + m_ePage, LogLevel.NORMAL);
		m_aPlayers.Clear();
		ClearButtons();

		array<string> rows = {};
		packedPlayers.Split("\n", rows, false);

		// Player page: max 4 rows (leaves Button4-11 for 8 action buttons).
		// Teleport-select page: max 8 rows (leaves Button8-9 for Confirm+Back).
		int maxPlayers = 8;
		if (m_ePage == ADM_AdminMenuPage.PLAYERS)
			maxPlayers = 4;

		int added = 0;
		for (int i = 0; i < rows.Count() && i < maxPlayers; i++)
		{
			if (rows[i].IsEmpty())
				continue;

			array<string> parts = {};
			rows[i].Split("|", parts, false);
			if (parts.Count() < 3)
				continue;

			int playerId = parts[0].ToInt();
			m_aPlayers.Insert(new ADM_PlayerListEntry(playerId, parts[1], parts[2]));
			AddButton("Button" + i, parts[1] + " [" + parts[2] + "]");
			added++;
		}

		Print(ADM_AdminMenuConstants.LOG_PREFIX + "HandlePlayerList " + added + " players parsed", LogLevel.NORMAL);

		if (m_ePage == ADM_AdminMenuPage.TELEPORT_SELECT_PLAYER)
		{
			string confirmLabel;
			if (m_eTeleportSelectMode == ADM_TeleportSelectMode.TO_PLAYER)
				confirmLabel = "Teleport me to selected";
			else
				confirmLabel = "Teleport selected to me";

			AddButton("Button8", confirmLabel);
			AddButton("Button9", "Back");

			if (m_aPlayers.IsEmpty())
				SetStatus("No other players online.");
			else if (m_eTeleportSelectMode == ADM_TeleportSelectMode.TO_PLAYER)
				SetStatus("Select a player, then click Teleport me to selected.");
			else
				SetStatus("Select a player, then click Teleport selected to me.");
		}
		else
		{
			AddButton("Button4",  "Kick");
			AddButton("Button5",  "Ban (perm)");
			AddButton("Button6",  "Unban");
			string gmLabel = "[OFF] God Mode";
			if (m_bClientGodMode)
				gmLabel = "[ON]  God Mode";
			AddButton("Button7",  gmLabel);
			AddButton("Button8",  "Heal");
			string pmAmmoLabel = "[OFF] Unlimited Ammo";
			if (m_bClientUnlimitedAmmo)
				pmAmmoLabel = "[ON]  Unlimited Ammo";
			AddButton("Button9",  pmAmmoLabel);
			string frLabel = "[OFF] Fast Run";
			if (m_bClientFastRun)
				frLabel = "[ON]  Fast Run";
			AddButton("Button10", frLabel);
			AddButton("Button11", "Back");
			SetStatus("Select a player row, then choose an action.");
		}

		m_iSelection = 0;
		RefreshSelection();
	}

	protected void HandleUnlimitedAmmoChanged(bool enabled)
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "HandleUnlimitedAmmoChanged enabled=" + enabled, LogLevel.NORMAL);
		m_bClientUnlimitedAmmo = enabled;
		RefreshActionButtonLabels();
		string ammoStatus = "OFF";
		if (enabled) ammoStatus = "ON";
		SetStatus("Unlimited Ammo: " + ammoStatus);
	}

	protected void HandleGodModeChanged(bool enabled)
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "HandleGodModeChanged enabled=" + enabled, LogLevel.NORMAL);
		m_bClientGodMode = enabled;
		m_mGodModeStates.Set(m_iSelectedPlayerId, enabled);
		RefreshActionButtonLabels();
		string gmStatus = "OFF";
		if (enabled) gmStatus = "ON";
		SetStatus("God Mode " + gmStatus + " for selected player.");
	}

	protected void HandleFastRunChanged(bool enabled)
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "HandleFastRunChanged enabled=" + enabled, LogLevel.NORMAL);
		m_bClientFastRun = enabled;
		RefreshActionButtonLabels();
		string frStatus = "OFF";
		if (enabled) frStatus = "ON";
		SetStatus("Fast Run: " + frStatus);
	}

	protected void RefreshActionButtonLabels()
	{
		if (m_ePage != ADM_AdminMenuPage.PLAYERS)
			return;

		int base = m_aPlayers.Count();

		int gmIdx = base + 3;
		if (gmIdx < m_aButtons.Count())
		{
			string gmLabel = "[OFF] God Mode";
			if (m_bClientGodMode) gmLabel = "[ON]  God Mode";
			TextWidget gmLbl = TextWidget.Cast(m_aButtons[gmIdx].FindAnyWidget("Label"));
			if (gmLbl) gmLbl.SetText(gmLabel);
		}

		int ammoIdx = base + 5;
		if (ammoIdx < m_aButtons.Count())
		{
			string ammoLabel = "[OFF] Unlimited Ammo";
			if (m_bClientUnlimitedAmmo) ammoLabel = "[ON]  Unlimited Ammo";
			TextWidget ammoLbl = TextWidget.Cast(m_aButtons[ammoIdx].FindAnyWidget("Label"));
			if (ammoLbl) ammoLbl.SetText(ammoLabel);
		}

		int frIdx = base + 6;
		if (frIdx < m_aButtons.Count())
		{
			string frLabel = "[OFF] Fast Run";
			if (m_bClientFastRun) frLabel = "[ON]  Fast Run";
			TextWidget frLbl = TextWidget.Cast(m_aButtons[frIdx].FindAnyWidget("Label"));
			if (frLbl) frLbl.SetText(frLabel);
		}
	}

	//---------------------------------------------------------------------------------------------
	// Selection dispatch

	protected void ActivateSelection()
	{
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ActivateSelection page=" + m_ePage + " sel=" + m_iSelection, LogLevel.NORMAL);

		SCR_PlayerController controller = GetLocalController();
		if (!controller)
		{
			Print(ADM_AdminMenuConstants.LOG_PREFIX + "ActivateSelection no local controller", LogLevel.WARNING);
			return;
		}

		// -- Login -------------------------------------------------------------
		if (m_ePage == ADM_AdminMenuPage.LOGIN)
		{
			if (m_iSelection == 0)
			{
				string pw = "";
				if (m_wInputA)
					pw = m_wInputA.GetText();
				Print(ADM_AdminMenuConstants.LOG_PREFIX + "Login attempt pw.len=" + pw.Length(), LogLevel.NORMAL);
				controller.ADM_RequestLogin(pw);
			}
			else
				Close();
			return;
		}

		// -- Main --------------------------------------------------------------
		if (m_ePage == ADM_AdminMenuPage.MAIN)
		{
			switch (m_iSelection)
			{
				case 0: ShowPlayers(); break;
				case 1: ShowTeleport(); break;
				case 2: ShowSpawn(); break;
				case 3:
					controller.ADM_RequestKillNearbyNPCs();
					SetStatus("Killing all nearby NPCs within 150 m...");
					break;
				default: Close(); break;
			}
			return;
		}

		// -- Player management -------------------------------------------------
		if (m_ePage == ADM_AdminMenuPage.PLAYERS)
		{
			if (m_iSelection < m_aPlayers.Count())
			{
				int newId = m_aPlayers[m_iSelection].m_iPlayerId;
				if (newId != m_iSelectedPlayerId)
				{
					m_iSelectedPlayerId   = newId;
					m_sSelectedIdentityId = m_aPlayers[m_iSelection].m_sIdentityId;
					if (m_mGodModeStates.Contains(m_iSelectedPlayerId))
						m_bClientGodMode = m_mGodModeStates.Get(m_iSelectedPlayerId);
					else
						m_bClientGodMode = false;
					RefreshActionButtonLabels();
				}
				SetStatus("Selected: " + m_aPlayers[m_iSelection].m_sName + " - choose an action below.");
			}
			else
			{
				int actionIdx = m_iSelection - m_aPlayers.Count();
				switch (actionIdx)
				{
					case 0:
						Print(ADM_AdminMenuConstants.LOG_PREFIX + "Kick player " + m_iSelectedPlayerId, LogLevel.NORMAL);
						controller.ADM_RequestModeration(m_iSelectedPlayerId, m_sSelectedIdentityId, ADM_AdminModerationAction.KICK);
						break;
					case 1:
						Print(ADM_AdminMenuConstants.LOG_PREFIX + "Ban player " + m_iSelectedPlayerId, LogLevel.NORMAL);
						controller.ADM_RequestModeration(m_iSelectedPlayerId, m_sSelectedIdentityId, ADM_AdminModerationAction.BAN, 0);
						break;
					case 2:
						Print(ADM_AdminMenuConstants.LOG_PREFIX + "Unban player " + m_iSelectedPlayerId, LogLevel.NORMAL);
						controller.ADM_RequestModeration(m_iSelectedPlayerId, m_sSelectedIdentityId, ADM_AdminModerationAction.UNBAN);
						break;
					case 3:
						Print(ADM_AdminMenuConstants.LOG_PREFIX + "God mode toggle for player " + m_iSelectedPlayerId, LogLevel.NORMAL);
						controller.ADM_RequestGodModePlayer(m_iSelectedPlayerId);
						SetStatus("God Mode toggling for selected player...");
						break;
					case 4:
						Print(ADM_AdminMenuConstants.LOG_PREFIX + "Heal player " + m_iSelectedPlayerId, LogLevel.NORMAL);
						controller.ADM_RequestHealPlayer(m_iSelectedPlayerId);
						SetStatus("Heal sent to selected player.");
						break;
					case 5:
						Print(ADM_AdminMenuConstants.LOG_PREFIX + "Toggle unlimited ammo (self)", LogLevel.NORMAL);
						controller.ADM_RequestToggleUnlimitedAmmo();
						break;
					case 6:
						Print(ADM_AdminMenuConstants.LOG_PREFIX + "Toggle fast run (self)", LogLevel.NORMAL);
						controller.ADM_RequestToggleFastRun();
						break;
					default:
						ShowMain();
						break;
				}
			}
			return;
		}

		// -- Teleport ----------------------------------------------------------
		if (m_ePage == ADM_AdminMenuPage.TELEPORT)
		{
			switch (m_iSelection)
			{
				case 0: ShowLocations(); break;
				case 1: ShowLocationsChernarus(); break;
				case 2: ShowTeleportSelectPlayer(ADM_TeleportSelectMode.TO_PLAYER); break;
				case 3: ShowTeleportSelectPlayer(ADM_TeleportSelectMode.PLAYER_TO_ME); break;
				case 4:
					vector mapPos;
					if (TryGetMapCursorPosition(mapPos))
					{
						controller.ADM_RequestTeleportSelf(mapPos);
						SetStatus("Teleporting to map cursor position...");
					}
					else
						SetStatus("Open the map first and hover the cursor over the terrain.");
					break;
				default: ShowMain(); break;
			}
			return;
		}

		// -- Teleport player selector ------------------------------------------
		if (m_ePage == ADM_AdminMenuPage.TELEPORT_SELECT_PLAYER)
		{
			int confirmIdx = m_aPlayers.Count();

			if (m_iSelection < m_aPlayers.Count())
			{
				m_iSelectedPlayerId   = m_aPlayers[m_iSelection].m_iPlayerId;
				m_sSelectedIdentityId = m_aPlayers[m_iSelection].m_sIdentityId;
				SetStatus("Selected: " + m_aPlayers[m_iSelection].m_sName + " - click Confirm.");
			}
			else if (m_iSelection == confirmIdx)
			{
				if (m_eTeleportSelectMode == ADM_TeleportSelectMode.TO_PLAYER)
					controller.ADM_RequestTeleportSelfToPlayer(m_iSelectedPlayerId);
				else
					controller.ADM_RequestTeleportPlayerToMe(m_iSelectedPlayerId);
			}
			else
				ShowTeleport();
			return;
		}

		// -- Spawn -------------------------------------------------------------
		if (m_ePage == ADM_AdminMenuPage.SPAWN)
		{
			string prefab = "";
			if (m_wInputA)
				prefab = m_wInputA.GetText();

			switch (m_iSelection)
			{
				case 0:
					Print(ADM_AdminMenuConstants.LOG_PREFIX + "Spawn item at feet prefab=" + prefab, LogLevel.NORMAL);
					controller.ADM_RequestSpawnItem(prefab, false);
					break;
				case 1:
					Print(ADM_AdminMenuConstants.LOG_PREFIX + "Spawn item to inventory prefab=" + prefab, LogLevel.NORMAL);
					controller.ADM_RequestSpawnItem(prefab, true);
					break;
				case 2: ShowSpawnItems(); break;
				case 3: ShowSpawnNPCs(); break;
				case 4:
					Print(ADM_AdminMenuConstants.LOG_PREFIX + "Spawn AI group at me", LogLevel.NORMAL);
					controller.ADM_RequestSpawnAIAtMe();
					break;
				case 5:
				{
					float bx = 0;
					float bz = 0;
					if (m_wInputB) bx = m_wInputB.GetText().ToFloat();
					if (m_wInputC) bz = m_wInputC.GetText().ToFloat();
					Print(ADM_AdminMenuConstants.LOG_PREFIX + "Spawn AI group at " + bx + " 0 " + bz, LogLevel.NORMAL);
					controller.ADM_RequestSpawnAI(ADM_AdminMenuConstants.DEFAULT_AI_GROUP_PREFAB, Vector(bx, 0, bz));
					break;
				}
				case 6:
					Print(ADM_AdminMenuConstants.LOG_PREFIX + "Spawn aggro USSR pair at me", LogLevel.NORMAL);
					controller.ADM_RequestSpawnAggroUnit();
					SetStatus("Aggro USSR pair spawning...");
					break;
				case 7:
					Print(ADM_AdminMenuConstants.LOG_PREFIX + "Spawn HMMWV", LogLevel.NORMAL);
					controller.ADM_RequestSpawnVehicle(ADM_ItemPrefabs.HMMWV);
					SetStatus("HMMWV spawning next to you...");
					break;
				case 8:
					Print(ADM_AdminMenuConstants.LOG_PREFIX + "Spawn UAZ-469", LogLevel.NORMAL);
					controller.ADM_RequestSpawnVehicle(ADM_ItemPrefabs.UAZ469);
					SetStatus("UAZ-469 spawning next to you...");
					break;
				default: ShowMain(); break;
			}
			return;
		}

		// -- Preset items weapons and medical ----------------------------------
		if (m_ePage == ADM_AdminMenuPage.SPAWN_ITEMS)
		{
			switch (m_iSelection)
			{
				case 0:  SelectItemAndReturn(ADM_ItemPrefabs.M16A2,         "M16A2"); break;
				case 1:  SelectItemAndReturn(ADM_ItemPrefabs.AK74,          "AK-74"); break;
				case 2:  SelectItemAndReturn(ADM_ItemPrefabs.AKS74U,        "AKS74U"); break;
				case 3:  SelectItemAndReturn(ADM_ItemPrefabs.SVD,           "SVD Sniper"); break;
				case 4:  SelectItemAndReturn(ADM_ItemPrefabs.M9,            "M9 Pistol"); break;
				case 5:  SelectItemAndReturn(ADM_ItemPrefabs.PM,            "PM Pistol"); break;
				case 6:  SelectItemAndReturn(ADM_ItemPrefabs.RPG7,          "RPG-7"); break;
				case 7:  SelectItemAndReturn(ADM_ItemPrefabs.PKM,           "PKM Machine Gun"); break;
				case 8:  SelectItemAndReturn(ADM_ItemPrefabs.MedkitUS,      "Medical Kit US"); break;
				case 9:  SelectItemAndReturn(ADM_ItemPrefabs.SalineBagUS,   "Saline Bag US"); break;
				case 10: ShowSpawnItems2(); break;
				default: ShowSpawn(); break;
			}
			return;
		}

		// -- Preset items vehicles and equipment -------------------------------
		if (m_ePage == ADM_AdminMenuPage.SPAWN_ITEMS2)
		{
			switch (m_iSelection)
			{
				case 0:  SelectItemAndReturn(ADM_ItemPrefabs.HMMWV,         "M998 HMMWV"); break;
				case 1:  SelectItemAndReturn(ADM_ItemPrefabs.UAZ469,        "UAZ-469"); break;
				case 2:  SelectItemAndReturn(ADM_ItemPrefabs.Huey,          "UH-1H Huey"); break;
				case 3:  SelectItemAndReturn(ADM_ItemPrefabs.Mi8MT,         "Mi-8MT"); break;
				case 4:  SelectItemAndReturn(ADM_ItemPrefabs.M923A1,        "M923A1 Truck"); break;
				case 5:  SelectItemAndReturn(ADM_ItemPrefabs.Ural4320,      "Ural-4320 Truck"); break;
				case 6:  SelectItemAndReturn(ADM_ItemPrefabs.BinocsUS,      "Binoculars M22"); break;
				case 7:  SelectItemAndReturn(ADM_ItemPrefabs.RadioUS,       "Radio AN/PRC-68"); break;
				case 8:  SelectItemAndReturn(ADM_ItemPrefabs.FieldDressing, "Field Dressing US"); break;
				case 9:  SelectItemAndReturn(ADM_ItemPrefabs.TourniquetUS,  "Tourniquet US"); break;
				case 10: ShowSpawnItems(); break;
				default: ShowSpawn(); break;
			}
			return;
		}

		// -- Chernarus towns page 1 -------------------------------------------
		if (m_ePage == ADM_AdminMenuPage.LOCATIONS_CHERNARUS)
		{
			switch (m_iSelection)
			{
				case 0:  controller.ADM_RequestTeleportSelf(Vector(7450, 0, 2350)); break; // Chernogorsk
				case 1:  controller.ADM_RequestTeleportSelf(Vector(11023, 0, 2281)); break; // Elektrozavodsk
				case 2:  controller.ADM_RequestTeleportSelf(Vector(11848, 0, 9062)); break; // Berezino
				case 3:  controller.ADM_RequestTeleportSelf(Vector(2648, 0, 2476)); break; // Zelenogorsk
				case 4:  controller.ADM_RequestTeleportSelf(Vector(11130, 0, 11015)); break; // Krasnostav
				case 5:  controller.ADM_RequestTeleportSelf(Vector(11694, 0, 5576)); break; // Solnichniy
				case 6:  controller.ADM_RequestTeleportSelf(Vector(11017, 0, 3402)); break; // Kamyshovo
				case 7:  controller.ADM_RequestTeleportSelf(Vector(7185, 0, 5583)); break; // Mogilevka
				case 8:  controller.ADM_RequestTeleportSelf(Vector(6001, 0, 6695)); break; // Stary Sobor
				case 9:  controller.ADM_RequestTeleportSelf(Vector(4731, 0, 9213)); break; // Vybor
				case 10: ShowLocationsChernarus2(); break;
				default: ShowTeleport(); break;
			}
			return;
		}

		// -- Chernarus towns page 2 + airfields --------------------------------
		if (m_ePage == ADM_AdminMenuPage.LOCATIONS_CHERNARUS_2)
		{
			switch (m_iSelection)
			{
				case 0: controller.ADM_RequestTeleportSelf(Vector(6425, 0, 7752)); break; // Novy Sobor
				case 1: controller.ADM_RequestTeleportSelf(Vector(5218, 0, 7773)); break; // Kabanino
				case 2: controller.ADM_RequestTeleportSelf(Vector(3632, 0, 5882)); break; // Rogovo
				case 3: controller.ADM_RequestTeleportSelf(Vector(3385, 0, 4368)); break; // Pavlovo
				case 4: controller.ADM_RequestTeleportSelf(Vector(12602, 0, 11014)); break; // Svetlojarsk
				case 5: controller.ADM_RequestTeleportSelf(Vector(4833, 0, 10467)); break; // NWAF
				case 6: controller.ADM_RequestTeleportSelf(Vector(6146, 0, 1724)); break; // Balota Airfield
				case 7: controller.ADM_RequestTeleportSelf(Vector(11160, 0, 11510)); break; // Krasnostav Airfield
				case 8: ShowLocationsChernarus(); break; // Button10 → array idx 8
				default: ShowTeleport(); break;
			}
			return;
		}

		// -- NPC browser -------------------------------------------------------
		if (m_ePage == ADM_AdminMenuPage.SPAWN_NPCS)
		{
			switch (m_iSelection)
			{
				case 0:
					controller.ADM_RequestSpawnNPCAtMe("{30ED11AA4F0D41E5}Prefabs/Groups/OPFOR/Group_USSR_FireGroup.et");
					SetStatus("USSR Fire Pair spawned next to you.");
					break;
				case 1:
					controller.ADM_RequestSpawnNPCAtMe("{657590C1EC9E27D3}Prefabs/Groups/OPFOR/Group_USSR_LightFireTeam.et");
					SetStatus("USSR Light Fire Team spawned next to you.");
					break;
				case 2:
					controller.ADM_RequestSpawnNPCAtMe("{E552DABF3636C2AD}Prefabs/Groups/OPFOR/Group_USSR_RifleSquad.et");
					SetStatus("USSR Rifle Squad spawned next to you.");
					break;
				case 3:
					controller.ADM_RequestSpawnNPCAtMe("{1BB20A4B3A53D0F5}Prefabs/Groups/INDFOR/Group_FIA_LightFireTeam.et");
					SetStatus("FIA Light Team spawned next to you.");
					break;
				case 4:
					controller.ADM_RequestSpawnNPCAtMe("{22F33D3EC8F281AB}Prefabs/Groups/INDFOR/Group_FIA_MachineGunTeam.et");
					SetStatus("FIA Machine Gun Team spawned next to you.");
					break;
				case 5:
					controller.ADM_RequestSpawnNPCAtMe("{2E9C920C3ACA2C6F}Prefabs/Groups/INDFOR/Group_FIA_ReconTeam.et");
					SetStatus("FIA Recon Team spawned next to you.");
					break;
				case 6:
					controller.ADM_RequestSpawnNPCAtMe("{E940C0DE00000007}Prefabs/Groups/EP4_ZombieGroup.et");
					SetStatus("Zombie Pack spawned next to you.");
					break;
				default: ShowSpawn(); break;
			}
			return;
		}

		// -- Locations towns page 1 --------------------------------------------
		if (m_ePage == ADM_AdminMenuPage.LOCATIONS)
		{
			switch (m_iSelection)
			{
				case 0:  controller.ADM_RequestTeleportSelf(Vector(5140, 0, 3980)); break; // Morton
				case 1:  controller.ADM_RequestTeleportSelf(Vector(7190, 0, 2320)); break; // Regina
				case 2:  controller.ADM_RequestTeleportSelf(Vector(8830, 0, 2730)); break; // Durras
				case 3:  controller.ADM_RequestTeleportSelf(Vector(9690, 0, 1590)); break; // Saint Pierre Harbour
				case 4:  controller.ADM_RequestTeleportSelf(Vector(5600, 0, 1500)); break; // Montignac
				case 5:  controller.ADM_RequestTeleportSelf(Vector(2200, 0, 2500)); break; // Le Moule
				case 6:  controller.ADM_RequestTeleportSelf(Vector(4000, 0, 5200)); break; // Almara
				case 7:  controller.ADM_RequestTeleportSelf(Vector(4100, 0, 7700)); break; // Chapoi
				case 8:  controller.ADM_RequestTeleportSelf(Vector(5700, 0, 6600)); break; // Houdan
				case 9:  controller.ADM_RequestTeleportSelf(Vector(7100, 0, 6000)); break; // Terria
				case 10: ShowLocations2(); break;
				default: ShowTeleport(); break;
			}
			return;
		}

		// -- Locations page 2 (more towns + airfields) -------------------------
		if (m_ePage == ADM_AdminMenuPage.LOCATIONS_2)
		{
			switch (m_iSelection)
			{
				case 0:  controller.ADM_RequestTeleportSelf(Vector(8800, 0, 3900)); break; // Briars
				case 1:  controller.ADM_RequestTeleportSelf(Vector(8400, 0, 5200)); break; // Tyrone
				case 2:  controller.ADM_RequestTeleportSelf(Vector(6100, 0, 4700)); break; // Vaux
				case 3:  controller.ADM_RequestTeleportSelf(Vector(5100, 0, 5700)); break; // Saint Philippe
				case 4:  controller.ADM_RequestTeleportSelf(Vector(1600, 0, 2800)); break; // Le Moule Airfield
				case 5:  controller.ADM_RequestTeleportSelf(Vector(9000, 0, 5700)); break; // Tyrone Airfield
				case 6:  ShowLocations(); break; // Button10 is at array index 6 (Button6-9 are skipped)
				default: ShowTeleport(); break;
			}
			return;
		}

		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ActivateSelection unhandled page " + m_ePage, LogLevel.WARNING);
	}

	//---------------------------------------------------------------------------------------------
	// Map cursor

	protected bool TryGetMapCursorPosition(out vector position)
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity)
		{
			Print(ADM_AdminMenuConstants.LOG_PREFIX + "TryGetMapCursorPosition no map instance", LogLevel.NORMAL);
			return false;
		}

		float worldX = 0;
		float worldZ = 0;
		mapEntity.GetMapCursorWorldPosition(worldX, worldZ);

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		position = Vector(worldX, world.GetSurfaceY(worldX, worldZ) + 0.5, worldZ);
		Print(ADM_AdminMenuConstants.LOG_PREFIX + "TryGetMapCursorPosition " + position, LogLevel.NORMAL);
		return true;
	}

	//---------------------------------------------------------------------------------------------
	// Keyboard navigation

	protected void OnMenuUp(float value, EActionTrigger trigger)
	{
		if (m_aButtons.IsEmpty()) return;
		m_iSelection--;
		if (m_iSelection < 0)
			m_iSelection = m_aButtons.Count() - 1;
		RefreshSelection();
	}

	protected void OnMenuDown(float value, EActionTrigger trigger)
	{
		if (m_aButtons.IsEmpty()) return;
		m_iSelection++;
		if (m_iSelection >= m_aButtons.Count())
			m_iSelection = 0;
		RefreshSelection();
	}

	protected void OnMenuSelect(float value, EActionTrigger trigger)
	{
		ActivateSelection();
	}

	protected void OnNextField(float value, EActionTrigger trigger)
	{
		FocusNextWidget();
	}

	protected void FocusNextWidget()
	{
		array<Widget> focusable = {};

		if (m_ePage == ADM_AdminMenuPage.LOGIN && m_wInputA)
			focusable.Insert(m_wInputA);
		else if (m_ePage == ADM_AdminMenuPage.TELEPORT)
		{
			if (m_wInputA) focusable.Insert(m_wInputA);
			if (m_wInputB) focusable.Insert(m_wInputB);
			if (m_wInputC) focusable.Insert(m_wInputC);
		}
		else if (m_wInputA)
			focusable.Insert(m_wInputA);

		foreach (ButtonWidget button : m_aButtons)
			focusable.Insert(button);

		if (focusable.IsEmpty())
			return;

		m_iFocusCursor++;
		if (m_iFocusCursor >= focusable.Count())
			m_iFocusCursor = 0;

		Widget focused = focusable[m_iFocusCursor];
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
			workspace.SetFocusedWidget(focused, true);

		for (int i = 0; i < m_aButtons.Count(); i++)
		{
			if (focused == m_aButtons[i])
			{
				m_iSelection = i;
				RefreshSelection();
				break;
			}
		}
	}

	//---------------------------------------------------------------------------------------------
	// Mouse click

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		for (int i = 0; i < m_aButtons.Count(); i++)
		{
			if (w == m_aButtons[i] || w.GetParent() == m_aButtons[i])
			{
				Print(ADM_AdminMenuConstants.LOG_PREFIX + "Mouse click button " + i, LogLevel.NORMAL);
				m_iSelection = i;
				RefreshSelection();
				ActivateSelection();
				return true;
			}
		}

		return false;
	}

	//---------------------------------------------------------------------------------------------
	// Widget geometry helpers (mirrors EP4_AdminMenuOverlay.ApplyLayoutGeometry pattern)

	protected void ADM_ApplyGeometry(Widget panel, Widget panelBg)
	{
		if (panel)
		{
			FrameSlot.SetAnchor(panel, 0.5, 0.5);
			FrameSlot.SetPos(panel, -280, -390);
			FrameSlot.SetSize(panel, 560, 780);
		}

		if (panelBg)
		{
			FrameSlot.SetAnchorMin(panelBg, 0, 0);
			FrameSlot.SetAnchorMax(panelBg, 1, 1);
			FrameSlot.SetOffsets(panelBg, 0, 0, 0, 0);
		}

		ADM_SetFrameRect(m_wTitle,  24,  18, 510, 44);
		ADM_SetFrameRect(m_wStatus, 24,  64, 510, 46);
		ADM_SetFrameRect(m_wInputA, 24, 122, 510, 40);
		ADM_SetFrameRect(m_wInputB, 24, 172, 245, 40);
		ADM_SetFrameRect(m_wInputC, 289, 172, 245, 40);

		for (int i = 0; i < 12; i++)
		{
			float y = 232 + i * 44;
			ADM_SetFrameRect(m_wRoot.FindAnyWidget("Button" + i.ToString()), 24, y, 510, 38);
		}

		Print(ADM_AdminMenuConstants.LOG_PREFIX + "ADM_ApplyGeometry() done", LogLevel.NORMAL);
	}

	protected void ADM_SetFrameRect(Widget w, float x, float y, float width, float height)
	{
		if (!w)
			return;
		FrameSlot.SetAnchor(w, 0, 0);
		FrameSlot.SetPos(w, x, y);
		FrameSlot.SetSize(w, width, height);
		w.SetOpacity(1);
	}
}
