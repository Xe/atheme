/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the ChanServ CLEAR USERS function.
 *
 * $Id: clear_users.c 7969 2007-03-23 19:19:38Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/clear_users", FALSE, _modinit, _moddeinit,
	"$Id: clear_users.c 7969 2007-03-23 19:19:38Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_clear_users(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_clear_users = { "USERS", N_("Kicks all users from a channel."),
	AC_NONE, 2, cs_cmd_clear_users };

list_t *cs_clear_cmds;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_clear_cmds, "chanserv/clear", "cs_clear_cmds");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

	command_add(&cs_clear_users, cs_clear_cmds);
	help_addentry(cs_helptree, "CLEAR USERS", "help/cservice/clear_users", NULL);
}

void _moddeinit()
{
	command_delete(&cs_clear_users, cs_clear_cmds);

	help_delentry(cs_helptree, "CLEAR USERS");
}

static void cs_cmd_clear_users(sourceinfo_t *si, int parc, char *parv[])
{
	char fullreason[200];
	channel_t *c;
	char *channel = parv[0];
	mychan_t *mc = mychan_find(channel);
	chanuser_t *cu;
	node_t *n, *tn;
	int oldlimit;

	if (parc >= 2)
		snprintf(fullreason, sizeof fullreason, "CLEAR USERS used by %s: %s", get_source_name(si), parv[1]);
	else
		snprintf(fullreason, sizeof fullreason, "CLEAR USERS used by %s", get_source_name(si));

	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!(c = channel_find(channel)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is currently empty."), channel);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_RECOVER))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), channel);
		return;
	}

	/* stop a race condition where users can rejoin */
	oldlimit = c->limit;
	if (oldlimit != 1)
		modestack_mode_limit(chansvs.nick, c, MTYPE_ADD, 1);
	modestack_flush_channel(c);

	LIST_FOREACH_SAFE(n, tn, c->members.head)
	{
		cu = n->data;

		/* don't kick the user who requested the masskick */
		if (cu->user == si->su || is_internal_client(cu->user))
			continue;

		kick(chansvs.nick, c->name, cu->user->nick, fullreason);
	}

	/* the channel may be empty now, so our pointer may be bogus! */
	c = channel_find(channel);
	if (c != NULL)
	{
		if ((mc->flags & MC_GUARD) && !config_options.leave_chans
				&& c != NULL && !chanuser_find(c, si->su))
		{
			/* Always cycle it if the requester is not on channel
			 * -- jilles */
			part(channel, chansvs.nick);
		}
		/* could be permanent channel, blah */
		c = channel_find(channel);
		if (c != NULL)
		{
			if (oldlimit == 0)
				modestack_mode_limit(chansvs.nick, c, MTYPE_DEL, 0);
			else if (oldlimit != 1)
				modestack_mode_limit(chansvs.nick, c, MTYPE_ADD, oldlimit);
		}
	}

	logcommand(si, CMDLOG_DO, "%s CLEAR USERS", mc->name);

	command_success_nodata(si, _("Cleared users from \2%s\2."), channel);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
