modded class ArmaReforgerScripted
{
	protected bool m_bADMF9WasDown;

	override void OnUpdate(BaseWorld world, float timeslice)
	{
		super.OnUpdate(world, timeslice);
		if (!System.IsConsoleApp())
		{
			ADM_PollToggleKey();
			ADM_AdminMenuOverlay.PollNavKeys();
		}
	}

	protected void ADM_PollToggleKey()
	{
		bool pressed = Debug.KeyState(KeyCode.KC_PRIOR);
		if (!pressed) { m_bADMF9WasDown = false; return; }
		if (m_bADMF9WasDown) return;
		m_bADMF9WasDown = true;
		Debug.ClearKey(KeyCode.KC_PRIOR);
		ADM_AdminMenuOverlay.Toggle();
	}
}
