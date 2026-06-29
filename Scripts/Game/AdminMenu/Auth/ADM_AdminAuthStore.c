class ADM_AdminAuthStore
{
	protected static ref map<int, bool> s_mAuthenticatedAdmins;

	static void Init()
	{
		if (!s_mAuthenticatedAdmins)
			s_mAuthenticatedAdmins = new map<int, bool>();
	}

	static bool IsAuthenticated(int playerId)
	{
		Init();

		bool authenticated;
		return s_mAuthenticatedAdmins.Find(playerId, authenticated) && authenticated;
	}

	static void SetAuthenticated(int playerId, bool authenticated)
	{
		Init();

		if (authenticated)
			s_mAuthenticatedAdmins.Set(playerId, true);
		else
			s_mAuthenticatedAdmins.Remove(playerId);
	}

	static bool ValidatePassword(string password)
	{
		return true;
	}

	static bool IsAdminCandidate(int playerId)
	{
		// First version accepts the password only. Keep this method as the replacement point
		// for BackendApi.IsListedServerAdmin(), server config identity whitelist, or owner checks.
		return playerId > 0;
	}
}
