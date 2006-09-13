/*
 * Copyright (c) 2006 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET command.
 *
 * $Id: set.c 6367 2006-09-13 00:34:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/set", FALSE, _modinit, _moddeinit,
	"$Id: set.c 6367 2006-09-13 00:34:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_set(sourceinfo_t *si, int parc, char *parv[]);

list_t *us_cmdtree, *us_helptree;

command_t us_set = { "SET", "Sets various control flags.", AC_NONE, 2, us_cmd_set };

list_t us_set_cmdtree;

/* SET <setting> <parameters> */
static void us_cmd_set(sourceinfo_t *si, int parc, char *parv[])
{
	char *setting = parv[0];
	command_t *c;

	if (si->su->myuser == NULL)
	{
		notice(usersvs.nick, si->su->nick, "You are not logged in.");
		return;
	}

	if (setting == NULL)
	{
		notice(usersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "SET");
		notice(usersvs.nick, si->su->nick, "Syntax: SET <setting> <parameters>");
		return;
	}

	/* take the command through the hash table */
        if ((c = command_find(&us_set_cmdtree, setting)))
	{
		command_exec(usersvs.me, si, c, parc - 1, parv + 1);
	}
	else
	{
		notice(usersvs.nick, si->su->nick, "Invalid set command. Use \2/%s%s HELP SET\2 for a command listing.", (ircd->uses_rcommand == FALSE) ? "msg " : "", usersvs.nick);
	}
}

/* SET EMAIL <new address> */
static void _us_setemail(sourceinfo_t *si, int parc, char *parv[])
{
	char *email = parv[0];

	if (si->su->myuser == NULL)
		return;

	if (email == NULL)
	{
		notice(usersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "EMAIL");
		notice(usersvs.nick, si->su->nick, "Syntax: SET EMAIL <new e-mail>");
		return;
	}

	if (strlen(email) >= EMAILLEN)
	{
		notice(usersvs.nick, si->su->nick, STR_INVALID_PARAMS, "EMAIL");
		return;
	}

	if (si->su->myuser->flags & MU_WAITAUTH)
	{
		notice(usersvs.nick, si->su->nick, "Please verify your original registration before changing your e-mail address.");
		return;
	}

	if (!validemail(email))
	{
		notice(usersvs.nick, si->su->nick, "\2%s\2 is not a valid email address.", email);
		return;
	}

	if (!strcasecmp(si->su->myuser->email, email))
	{
		notice(usersvs.nick, si->su->nick, "The email address for \2%s\2 is already set to \2%s\2.", si->su->myuser->name, si->su->myuser->email);
		return;
	}

	snoop("SET:EMAIL: \2%s\2 (\2%s\2 -> \2%s\2)", si->su->myuser->name, si->su->myuser->email, email);

	if (me.auth == AUTH_EMAIL)
	{
		unsigned long key = makekey();

		metadata_add(si->su->myuser, METADATA_USER, "private:verify:emailchg:key", itoa(key));
		metadata_add(si->su->myuser, METADATA_USER, "private:verify:emailchg:newemail", email);
		metadata_add(si->su->myuser, METADATA_USER, "private:verify:emailchg:timestamp", itoa(time(NULL)));

		if (!sendemail(si->su, EMAIL_SETEMAIL, si->su->myuser, itoa(key)))
		{
			notice(usersvs.nick, si->su->nick, "Sending email failed, sorry! Your email address is unchanged.");
			metadata_delete(si->su->myuser, METADATA_USER, "private:verify:emailchg:key");
			metadata_delete(si->su->myuser, METADATA_USER, "private:verify:emailchg:newemail");
			metadata_delete(si->su->myuser, METADATA_USER, "private:verify:emailchg:timestamp");
			return;
		}

		logcommand(usersvs.me, si->su, CMDLOG_SET, "SET EMAIL %s (awaiting verification)", email);
		notice(usersvs.nick, si->su->nick, "An email containing email changing instructions has been sent to \2%s\2.", email);
		notice(usersvs.nick, si->su->nick, "Your email address will not be changed until you follow these instructions.");

		return;
	}

	strlcpy(si->su->myuser->email, email, EMAILLEN);

	logcommand(usersvs.me, si->su, CMDLOG_SET, "SET EMAIL %s", email);
	notice(usersvs.nick, si->su->nick, "The email address for \2%s\2 has been changed to \2%s\2.", si->su->myuser->name, si->su->myuser->email);
}

command_t us_set_email = { "EMAIL", "Changes the e-mail address associated with a username.", AC_NONE, 1, _us_setemail };

/* SET HIDEMAIL [ON|OFF] */
static void _us_sethidemail(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = strtok(parv[0], " ");

	if (si->su->myuser == NULL)
		return;

	if (params == NULL)
	{
		notice(usersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "HIDEMAIL");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_HIDEMAIL & si->su->myuser->flags)
		{
			notice(usersvs.nick, si->su->nick, "The \2HIDEMAIL\2 flag is already set for \2%s\2.", si->su->myuser->name);
			return;
		}

		logcommand(usersvs.me, si->su, CMDLOG_SET, "SET HIDEMAIL ON");

		si->su->myuser->flags |= MU_HIDEMAIL;

		notice(usersvs.nick, si->su->nick, "The \2HIDEMAIL\2 flag has been set for \2%s\2.", si->su->myuser->name);

		return;
	}
	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_HIDEMAIL & si->su->myuser->flags))
		{
			notice(usersvs.nick, si->su->nick, "The \2HIDEMAIL\2 flag is not set for \2%s\2.", si->su->myuser->name);
			return;
		}

		logcommand(usersvs.me, si->su, CMDLOG_SET, "SET HIDEMAIL OFF");

		si->su->myuser->flags &= ~MU_HIDEMAIL;

		notice(usersvs.nick, si->su->nick, "The \2HIDEMAIL\2 flag has been removed for \2%s\2.", si->su->myuser->name);

		return;
	}

	else
	{
		notice(usersvs.nick, si->su->nick, STR_INVALID_PARAMS, "HIDEMAIL");
		return;
	}
}

command_t us_set_hidemail = { "HIDEMAIL", "Hides the e-mail address associated with a username.", AC_NONE, 1, _us_sethidemail };

static void _us_setemailmemos(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = strtok(parv[0], " ");

	if (si->su->myuser == NULL)
		return;

	if (si->su->myuser->flags & MU_WAITAUTH)
	{
		notice(usersvs.nick, si->su->nick, "You have to verify your email address before you can enable emailing memos.");
		return;
	}

	if (params == NULL)
	{
		notice(usersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "EMAILMEMOS");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (me.mta == NULL)
		{
			notice(usersvs.nick, si->su->nick, "Sending email is administratively disabled.");
			return;
		}
		if (MU_EMAILMEMOS & si->su->myuser->flags)
		{
			notice(usersvs.nick, si->su->nick, "The \2EMAILMEMOS\2 flag is already set for \2%s\2.", si->su->myuser->name);
			return;
		}

		logcommand(usersvs.me, si->su, CMDLOG_SET, "SET EMAILMEMOS ON");
		si->su->myuser->flags |= MU_EMAILMEMOS;
		notice(usersvs.nick, si->su->nick, "The \2EMAILMEMOS\2 flag has been set for \2%s\2.", si->su->myuser->name);
		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_EMAILMEMOS & si->su->myuser->flags))
		{
			notice(usersvs.nick, si->su->nick, "The \2EMAILMEMOS\2 flag is not set for \2%s\2.", si->su->myuser->name);
			return;
		}

		logcommand(usersvs.me, si->su, CMDLOG_SET, "SET EMAILMEMOS OFF");
		si->su->myuser->flags &= ~MU_EMAILMEMOS;
		notice(usersvs.nick, si->su->nick, "The \2EMAILMEMOS\2 flag has been removed for \2%s\2.", si->su->myuser->name);
		return;
	}

	else
	{
		notice(usersvs.nick, si->su->nick, STR_INVALID_PARAMS, "EMAILMEMOS");
		return;
	}
}

command_t us_set_emailmemos = { "EMAILMEMOS", "Forwards incoming memos to your account's e-mail address.", AC_NONE, 1, _us_setemailmemos };

static void _us_setnomemo(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = strtok(parv[0], " ");

	if (si->su->myuser == NULL)
		return;

	if (params == NULL)
	{
		notice(usersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "NOMEMO");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NOMEMO & si->su->myuser->flags)
		{
			notice(usersvs.nick, si->su->nick, "The \2NOMEMO\2 flag is already set for \2%s\2.", si->su->myuser->name);
			return;
		}

		logcommand(usersvs.me, si->su, CMDLOG_SET, "SET NOMEMO ON");
		si->su->myuser->flags |= MU_NOMEMO;
		notice(usersvs.nick, si->su->nick, "The \2NOMEMO\2 flag has been set for \2%s\2.", si->su->myuser->name);
		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NOMEMO & si->su->myuser->flags))
		{
			notice(usersvs.nick, si->su->nick, "The \2NOMEMO\2 flag is not set for \2%s\2.", si->su->myuser->name);
			return;
		}

		logcommand(usersvs.me, si->su, CMDLOG_SET, "SET NOMEMO OFF");
		si->su->myuser->flags &= ~MU_NOMEMO;
		notice(usersvs.nick, si->su->nick, "The \2NOMEMO\2 flag has been removed for \2%s\2.", si->su->myuser->name);
		return;
	}

	else
	{
		notice(usersvs.nick, si->su->nick, STR_INVALID_PARAMS, "NOMEMO");
		return;
	}
}

command_t us_set_nomemo = { "NOMEMO", "Disables the ability to recieve memos.", AC_NONE, 1, _us_setnomemo };

static void _us_setneverop(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = strtok(parv[0], " ");

	if (si->su->myuser == NULL)
		return;

	if (params == NULL)
	{
		notice(usersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "NEVEROP");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NEVEROP & si->su->myuser->flags)
		{
			notice(usersvs.nick, si->su->nick, "The \2NEVEROP\2 flag is already set for \2%s\2.", si->su->myuser->name);
			return;
		}

		logcommand(usersvs.me, si->su, CMDLOG_SET, "SET NEVEROP ON");

		si->su->myuser->flags |= MU_NEVEROP;

		notice(usersvs.nick, si->su->nick, "The \2NEVEROP\2 flag has been set for \2%s\2.", si->su->myuser->name);

		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NEVEROP & si->su->myuser->flags))
		{
			notice(usersvs.nick, si->su->nick, "The \2NEVEROP\2 flag is not set for \2%s\2.", si->su->myuser->name);
			return;
		}

		logcommand(usersvs.me, si->su, CMDLOG_SET, "SET NEVEROP OFF");

		si->su->myuser->flags &= ~MU_NEVEROP;

		notice(usersvs.nick, si->su->nick, "The \2NEVEROP\2 flag has been removed for \2%s\2.", si->su->myuser->name);

		return;
	}

	else
	{
		notice(usersvs.nick, si->su->nick, STR_INVALID_PARAMS, "NEVEROP");
		return;
	}
}

command_t us_set_neverop = { "NEVEROP", "Prevents you from being added to access lists.", AC_NONE, 1, _us_setneverop };

static void _us_setnoop(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = strtok(parv[0], " ");

	if (si->su->myuser == NULL)
		return;

	if (params == NULL)
	{
		notice(usersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "NOOP");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NOOP & si->su->myuser->flags)
		{
			notice(usersvs.nick, si->su->nick, "The \2NOOP\2 flag is already set for \2%s\2.", si->su->myuser->name);
			return;
		}

		logcommand(usersvs.me, si->su, CMDLOG_SET, "SET NOOP ON");

		si->su->myuser->flags |= MU_NOOP;

		notice(usersvs.nick, si->su->nick, "The \2NOOP\2 flag has been set for \2%s\2.", si->su->myuser->name);

		return;
	}
	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NOOP & si->su->myuser->flags))
		{
			notice(usersvs.nick, si->su->nick, "The \2NOOP\2 flag is not set for \2%s\2.", si->su->myuser->name);
			return;
		}

		logcommand(usersvs.me, si->su, CMDLOG_SET, "SET NOOP OFF");

		si->su->myuser->flags &= ~MU_NOOP;

		notice(usersvs.nick, si->su->nick, "The \2NOOP\2 flag has been removed for \2%s\2.", si->su->myuser->name);

		return;
	}

	else
	{
		notice(usersvs.nick, si->su->nick, STR_INVALID_PARAMS, "NOOP");
		return;
	}
}

command_t us_set_noop = { "NOOP", "Prevents services from setting modes upon you automatically.", AC_NONE, 1, _us_setnoop };

static void _us_setproperty(sourceinfo_t *si, int parc, char *parv[])
{
	char *property = strtok(parv[0], " ");
	char *value = strtok(NULL, "");

	if (si->su->myuser == NULL)
		return;

	if (!property)
	{
		notice(usersvs.nick, si->su->nick, "Syntax: SET PROPERTY <property> [value]");
		return;
	}

	if (strchr(property, ':') && !has_priv(si->su, PRIV_METADATA))
	{
		notice(usersvs.nick, si->su->nick, "Invalid property name.");
		return;
	}

	if (strchr(property, ':'))
		snoop("SET:PROPERTY: \2%s\2: \2%s\2/\2%s\2", si->su->myuser->name, property, value);

	if (si->su->myuser->metadata.count >= me.mdlimit)
	{
		notice(usersvs.nick, si->su->nick, "Cannot add \2%s\2 to \2%s\2 metadata table, it is full.",
					property, si->su->myuser->name);
		return;
	}

	if (!value)
	{
		metadata_t *md = metadata_find(si->su->myuser, METADATA_USER, property);

		if (!md)
		{
			notice(usersvs.nick, si->su->nick, "Metadata entry \2%s\2 was not set.", property);
			return;
		}

		metadata_delete(si->su->myuser, METADATA_USER, property);
		logcommand(usersvs.me, si->su, CMDLOG_SET, "SET PROPERTY %s (deleted)", property);
		notice(usersvs.nick, si->su->nick, "Metadata entry \2%s\2 has been deleted.", property);
		return;
	}

	if (strlen(property) > 32 || strlen(value) > 300)
	{
		notice(usersvs.nick, si->su->nick, "Parameters are too long. Aborting.");
		return;
	}

	metadata_add(si->su->myuser, METADATA_USER, property, value);
	logcommand(usersvs.me, si->su, CMDLOG_SET, "SET PROPERTY %s to %s", property, value);
	notice(usersvs.nick, si->su->nick, "Metadata entry \2%s\2 added.", property);
}

command_t us_set_property = { "PROPERTY", "Manipulates metadata entries associated with a username.", AC_NONE, 2, _us_setproperty };

static void _us_setpassword(sourceinfo_t *si, int parc, char *parv[])
{
	char *password = strtok(parv[0], " ");

	if (si->su->myuser == NULL)
		return;

	if (password == NULL)
	{
		notice(usersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "PASSWORD");
		return;
	}

	if (strlen(password) > 32)
	{
		notice(usersvs.nick, si->su->nick, STR_INVALID_PARAMS, "PASSWORD");
		return;
	}

	if (!strcasecmp(password, si->su->myuser->name))
	{
		notice(usersvs.nick, si->su->nick, "You cannot use your username as a password.");
		notice(usersvs.nick, si->su->nick, "Syntax: SET PASSWORD <new password>");
		return;
	}

	/*snoop("SET:PASSWORD: \2%s\2 as \2%s\2 for \2%s\2", si->su->user, si->su->myuser->name, si->su->myuser->name);*/
	logcommand(usersvs.me, si->su, CMDLOG_SET, "SET PASSWORD");

	set_password(si->su->myuser, password);

	notice(usersvs.nick, si->su->nick, "The password for \2%s\2 has been changed to \2%s\2. Please write this down for future reference.", si->su->myuser->name, password);

	return;
}

command_t us_set_password = { "PASSWORD", "Changes the password associated with your username.", AC_NONE, 1, _us_setpassword };

command_t *us_set_commands[] = {
	&us_set_email,
	&us_set_emailmemos,
	&us_set_hidemail,
	&us_set_nomemo,
	&us_set_noop,
	&us_set_neverop,
	&us_set_password,
	&us_set_property,
	NULL
};

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(us_cmdtree, "userserv/main", "us_cmdtree");
	MODULE_USE_SYMBOL(us_helptree, "userserv/main", "us_helptree");
	command_add(&us_set, us_cmdtree);

	help_addentry(us_helptree, "SET EMAIL", "help/userserv/set_email", NULL);
	help_addentry(us_helptree, "SET EMAILMEMOS", "help/userserv/set_emailmemos", NULL);
	help_addentry(us_helptree, "SET HIDEMAIL", "help/userserv/set_hidemail", NULL);
	help_addentry(us_helptree, "SET NOMEMO", "help/userserv/set_nomemo", NULL);
	help_addentry(us_helptree, "SET NEVEROP", "help/userserv/set_neverop", NULL);
	help_addentry(us_helptree, "SET NOOP", "help/userserv/set_noop", NULL);
	help_addentry(us_helptree, "SET PASSWORD", "help/userserv/set_password", NULL);
	help_addentry(us_helptree, "SET PROPERTY", "help/userserv/set_property", NULL);

	/* populate us_set_cmdtree */
	command_add_many(us_set_commands, &us_set_cmdtree);
}

void _moddeinit()
{
	command_delete(&us_set, us_cmdtree);
	help_delentry(us_helptree, "SET EMAIL");
	help_delentry(us_helptree, "SET EMAILMEMOS");
	help_delentry(us_helptree, "SET HIDEMAIL");
	help_delentry(us_helptree, "SET NOMEMO");
	help_delentry(us_helptree, "SET NEVEROP");
	help_delentry(us_helptree, "SET NOOP");
	help_delentry(us_helptree, "SET PASSWORD");
	help_delentry(us_helptree, "SET PROPERTY");

	/* clear us_set_cmdtree */
	command_delete_many(us_set_commands, &us_set_cmdtree);
}
