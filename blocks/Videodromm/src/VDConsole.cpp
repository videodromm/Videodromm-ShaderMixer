#include "VDConsole.h"

using namespace VideoDromm;

VDConsole::VDConsole(VDSettingsRef aVDSettings, VDSessionRef aVDSession)
{
	mVDSettings = aVDSettings;
	mVDSession = aVDSession;

	ClearLog();
	HistoryPos = -1;
	Commands.push_back("HELP");
	Commands.push_back("HISTORY");
	Commands.push_back("CLEAR");
	Commands.push_back("DEBUG");
	Commands.push_back("WARPCREATE");
	Commands.push_back("WARPF");
	Commands.push_back("WSCNX");
	Commands.push_back("WSPING");
	Commands.push_back("WSCNF");
	Commands.push_back("LOADSTOP");
	Commands.push_back("TEMPO");
	strcpy(InputBuf, "");
}
VDConsole::~VDConsole()
{
	ClearLog();
	for (int i = 0; i < History.Size; i++)
		free(History[i]);
}
void VDConsole::ClearLog()
{
	for (int i = 0; i < Items.Size; i++)
		free(Items[i]);
	Items.clear();
	ScrollToBottom = true;
}
void VDConsole::AddLog(const char* fmt, ...) IM_PRINTFARGS(2)
{
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
	buf[IM_ARRAYSIZE(buf) - 1] = 0;
	va_end(args);
	Items.push_back(Strdup(buf));
	ScrollToBottom = true;
}

void VDConsole::Run(const char* title)
{

	ui::SetNextWindowSize(ImVec2(mVDSettings->mRenderWidth / 2, mVDSettings->uiLargePreviewH * 2), ImGuiSetCond_Once);
	ui::SetNextWindowPos(ImVec2(mVDSettings->uiMargin, mVDSettings->uiYPosRow3), ImGuiSetCond_Once);

	if (!ImGui::Begin(title))
	{
		ImGui::End();
		return;
	}

	ImGui::TextWrapped("TAB=text completion");

	// TODO: display from bottom
	// TODO: clip manually

	if (ImGui::SmallButton("Help"))
	{
		AddLog("Commands:");
		for (size_t i = 0; i < Commands.size(); i++)
			AddLog("- %s", Commands[i]);
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Clear")) ClearLog();

	ImGui::Separator();

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	static ImGuiTextFilter filter;
	filter.Draw("Filter (\"incl,-excl\") (\"error\")", 70);
	//if (ImGui::IsItemHovered()) ImGui::SetKeyboardFocusHere(-1); // Auto focus on hover
	ImGui::PopStyleVar();
	ImGui::Separator();

	// Display every line as a separate entry so we can change their color or add custom widgets. If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
	// NB- if you have thousands of entries this approach may be too inefficient. You can seek and display only the lines that are visible - CalcListClipping() is a helper to compute this information.
	// If your items are of variable size you may want to implement code similar to what CalcListClipping() does. Or split your data into fixed height items to allow random-seeking into your list.
	ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing() * 2));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
	for (size_t i = 0; i < Items.size(); i++)
	{
		const char* item = Items[i];
		if (!filter.PassFilter(item))
			continue;
		ImVec4 col(1, 1, 1, 1); // A better implement may store a type per-item. For the sample let's just parse the text.
		if (strstr(item, "[error]")) col = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
		else if (strncmp(item, "# ", 2) == 0) col = ImVec4(1.0f, 0.8f, 0.6f, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, col);
		ImGui::TextUnformatted(item);
		ImGui::PopStyleColor();
	}
	if (ScrollToBottom)
		ImGui::SetScrollPosHere();
	ScrollToBottom = false;
	ImGui::PopStyleVar();
	ImGui::EndChild();
	ImGui::Separator();

	// Command-line
	/* STOPS EXEC IN RELEASE MODE (TO CHECK) */
	if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory, &TextEditCallbackStub, (void*)this))
	{
		char* input_end = InputBuf + strlen(InputBuf);
		while (input_end > InputBuf && input_end[-1] == ' ') input_end--; *input_end = 0;
		if (InputBuf[0])
			ExecCommand(InputBuf);
		strcpy(InputBuf, "");
	}

	// Demonstrate keeping auto focus on the input box
	if (ImGui::IsItemHovered() || (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)))
		ImGui::SetKeyboardFocusHere(-1); // Auto focus

	ImGui::End();
}
void VDConsole::ExecCommand(const char* command_line)
{
	AddLog("# %s\n", command_line);

	// Insert into history. First find match and delete it so it can be pushed to the back. This isn't trying to be smart or optimal.
	HistoryPos = -1;
	for (int i = (int)History.size() - 1; i >= 0; i--)
		if (Stricmp(History[i], command_line) == 0)
		{
			ImGui::MemFree(History[i]);
			History.erase(History.begin() + i);
			break;
		}
	History.push_back(Strdup(command_line));

	// Process command
	if (Stricmp(command_line, "CLEAR") == 0)
	{
		ClearLog();
	}
	else if (Stricmp(command_line, "HELP") == 0)
	{
		AddLog("Commands:");
		for (size_t i = 0; i < Commands.size(); i++)
			AddLog("- %s", Commands[i]);
	}
	else if (Stricmp(command_line, "HISTORY") == 0)
	{
		for (size_t i = History.size() >= 10 ? History.size() - 10 : 0; i < History.size(); i++)
			AddLog("%3d: %s\n", i, History[i]);
	}
	else if (Stricmp(command_line, "DEBUG") == 0)
	{
		mVDSettings->iDebug = !mVDSettings->iDebug;
	}
	else if (Stricmp(command_line, "LOADSTOP") == 0)
	{
		//mVDMix->stopLoading();
	}
	else if (Strnicmp(command_line, "TEMPO", 5) == 0)
	{
		if (strlen(command_line) > 7)
		{
			string to = command_line;
			//int firstDigit = to.find_first_of("0123456789");
			//if (firstDigit > -1) mVDSettings->mTempo = std::stoi(to.substr(firstDigit));
		}
	}
#if (defined(  CINDER_MSW) )
	else if (Stricmp(command_line, "WSCNX") == 0)
	{
		mVDSession->wsConnect();
	}
	else if (Stricmp(command_line, "WSPING") == 0)
	{
		mVDSession->wsPing();
	}
	else if (Stricmp(command_line, "WSCNF") == 0)
	{
		if (mVDSettings->mIsWebSocketsServer)
		{
			AddLog("Server %s\n", mVDSettings->mWebSocketsHost.c_str());
		}
		else
		{
			//AddLog("Client %s %s\n", mVDSettings->mWebSocketsHost.c_str(), (mWebSockets->isClientConnected() ? "true" : "false"));
			AddLog("Client %s \n", mVDSettings->mWebSocketsHost.c_str());
		}
	}
#endif
	else
	{
		AddLog("Unknown command: '%s'\n", command_line);
	}
}

int VDConsole::TextEditCallback(ImGuiTextEditCallbackData* data)
{
	//AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
	switch (data->EventFlag)
	{
	case ImGuiInputTextFlags_CallbackCompletion:
	{
		// Example of TEXT COMPLETION

		// Locate beginning of current word
		const char* word_end = data->Buf + data->CursorPos;
		const char* word_start = word_end;
		while (word_start > data->Buf)
		{
			const char c = word_start[-1];
			if (c == ' ' || c == '\t' || c == ',' || c == ';')
				break;
			word_start--;
		}

		// Build a list of candidates
		ImVector<const char*> candidates;
		for (size_t i = 0; i < Commands.size(); i++)
			if (Strnicmp(Commands[i], word_start, (int)(word_end - word_start)) == 0)
				candidates.push_back(Commands[i]);

		if (candidates.size() == 0)
		{
			// No match
			AddLog("No match for \"%.*s\"!\n", word_end - word_start, word_start);
		}
		else if (candidates.size() == 1)
		{
			// Single match. Delete the beginning of the word and replace it entirely so we've got nice casing
			data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
			data->InsertChars(data->CursorPos, candidates[0]);
			data->InsertChars(data->CursorPos, " ");
		}
		else
		{
			// Multiple matches. Complete as much as we can, so inputing "C" will complete to "CL" and display "CLEAR" and "CLASSIFY"
			int match_len = (int)(word_end - word_start);
			for (;;)
			{
				int c = 0;
				bool all_candidates_matches = true;
				for (size_t i = 0; i < candidates.size() && all_candidates_matches; i++)
					if (i == 0)
						c = toupper(candidates[i][match_len]);
					else if (c != toupper(candidates[i][match_len]))
						all_candidates_matches = false;
				if (!all_candidates_matches)
					break;
				match_len++;
			}

			if (match_len > 0)
			{
				data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
				data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
			}

			// List matches
			AddLog("Possible matches:\n");
			for (size_t i = 0; i < candidates.size(); i++)
				AddLog("- %s\n", candidates[i]);
		}

		break;
	}
	case ImGuiInputTextFlags_CallbackHistory:
	{
		// Example of HISTORY
		const int prev_history_pos = HistoryPos;
		if (data->EventKey == ImGuiKey_UpArrow)
		{
			if (HistoryPos == -1)
				HistoryPos = History.Size - 1;
			else if (HistoryPos > 0)
				HistoryPos--;
		}
		else if (data->EventKey == ImGuiKey_DownArrow)
		{
			if (HistoryPos != -1)
				if (++HistoryPos >= History.Size)
					HistoryPos = -1;
		}

		// A better implementation would preserve the data on the current input line along with cursor position.
		if (prev_history_pos != HistoryPos)
		{
			data->CursorPos = data->SelectionStart = data->SelectionEnd = data->BufSize = (int)snprintf(data->Buf, (size_t)data->BufSize, "%s", (HistoryPos >= 0) ? History[HistoryPos] : "");
			data->BufDirty = true;
		}
	}
	}
	return 0;
}
