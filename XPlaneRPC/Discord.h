#pragma once
/*
* To change this license header, choose License Headers in Project Properties.
* To change this template file, choose Tools | Templates
* and open the template in the editor.
*/

/*
* File:   Discord.h
* Author: Connor T
*
* Created on October 19, 2018, 8:43 PM
*/

#ifndef DISCORD_H
#define DISCORD_H

#include "discord_register.h"
#include "discord_rpc.h"
#include <ctime>

static void handleDiscordReady(const DiscordUser* connectedUser) {
	const char opening[] = "\nDiscord: connected to user ";
    XPLMDebugString(opening + *connectedUser->username + '#' + *connectedUser->discriminator + ' ' + *connectedUser->userId);
}

static void handleDiscordDisconnected(int errorCode, const char* message) {
    XPLMDebugString("\nDiscord: disconnected (" + errorCode + ', ' + *message + ')');
}

static void handleDiscordError(int errorCode, const char* message)
{
    XPLMDebugString("\nDiscord: error (" + errorCode + ', ' + *message + ')');
}

static void handleDiscordSpectateGame(const char* secret) {}
static void handleDiscordJoinGame(const char* secret) {}
static void handleDiscordJoinRequest(const DiscordUser* request) {}

static void InitDiscord()
{
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	handlers.ready = handleDiscordReady;
	handlers.errored = handleDiscordError;
	handlers.disconnected = handleDiscordDisconnected;
	handlers.joinGame = handleDiscordJoinGame;
	handlers.spectateGame = handleDiscordSpectateGame;
	handlers.joinRequest = handleDiscordJoinRequest;

	Discord_Initialize("503007469467860993", &handlers, NULL, NULL);
}

void UpdatePresence(const char* state, const char* details, time_t timestamp, boolean use_start, const char* text, const char* smalll, const char* small_text) {
	DiscordRichPresence presence;
	memset(&presence, 0, sizeof(presence));
	presence.state = state;
	presence.details = details;
	presence.largeImageKey = "logo";
	presence.largeImageText = text;
	presence.smallImageKey = smalll;
	presence.smallImageText = small_text;

	if (timestamp != NULL && use_start)
		presence.startTimestamp = timestamp;
	else
		presence.endTimestamp = timestamp;

	Discord_UpdatePresence(&presence);
}

#endif /* DISCORD_H */

