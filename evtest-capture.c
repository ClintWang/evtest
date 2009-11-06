/*
 *  Copyright (c) 2009 Red Hat, Inc.
 *
 *  Event device capture program.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/input.h>

#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)	((array[LONG(bit)] >> OFF(bit)) & 1)
#define MY_ENCODING "ISO-8859-1"

/* libxml api is annoying. 'sanitize' it with macros */
#define call(func) \
	rc = (func); \
	if (rc < 0) \
		goto error;
#define start(...) call(xmlTextWriterStartElement(writer, __VA_ARGS__))
#define end(dummy) call(xmlTextWriterEndElement(writer))
#define format(...) call(xmlTextWriterWriteFormatElement(writer, __VA_ARGS__))
#define attr(...) call(xmlTextWriterWriteAttribute(writer, __VA_ARGS__))

static int stop = 0;

const char *keys[KEY_MAX + 1] = {
	[KEY_RESERVED] = "KEY_RESERVED",
	[KEY_ESC] = "KEY_ESC",
	[KEY_1] = "KEY_1",
	[KEY_2] = "KEY_2",
	[KEY_3] = "KEY_3",
	[KEY_4] = "KEY_4",
	[KEY_5] = "KEY_5",
	[KEY_6] = "KEY_6",
	[KEY_7] = "KEY_7",
	[KEY_8] = "KEY_8",
	[KEY_9] = "KEY_9",
	[KEY_0] = "KEY_0",
	[KEY_MINUS] = "KEY_MINUS",
	[KEY_EQUAL] = "KEY_EQUAL",
	[KEY_BACKSPACE] = "KEY_BACKSPACE",
	[KEY_TAB] = "KEY_TAB",
	[KEY_Q] = "KEY_Q",
	[KEY_W] = "KEY_W",
	[KEY_E] = "KEY_E",
	[KEY_R] = "KEY_R",
	[KEY_T] = "KEY_T",
	[KEY_Y] = "KEY_Y",
	[KEY_U] = "KEY_U",
	[KEY_I] = "KEY_I",
	[KEY_O] = "KEY_O",
	[KEY_P] = "KEY_P",
	[KEY_LEFTBRACE] = "KEY_LEFTBRACE",
	[KEY_RIGHTBRACE] = "KEY_RIGHTBRACE",
	[KEY_ENTER] = "KEY_ENTER",
	[KEY_LEFTCTRL] = "KEY_LEFTCTRL",
	[KEY_A] = "KEY_A",
	[KEY_S] = "KEY_S",
	[KEY_D] = "KEY_D",
	[KEY_F] = "KEY_F",
	[KEY_G] = "KEY_G",
	[KEY_H] = "KEY_H",
	[KEY_J] = "KEY_J",
	[KEY_K] = "KEY_K",
	[KEY_L] = "KEY_L",
	[KEY_SEMICOLON] = "KEY_SEMICOLON",
	[KEY_APOSTROPHE] = "KEY_APOSTROPHE",
	[KEY_GRAVE] = "KEY_GRAVE",
	[KEY_LEFTSHIFT] = "KEY_LEFTSHIFT",
	[KEY_BACKSLASH] = "KEY_BACKSLASH",
	[KEY_Z] = "KEY_Z",
	[KEY_X] = "KEY_X",
	[KEY_C] = "KEY_C",
	[KEY_V] = "KEY_V",
	[KEY_B] = "KEY_B",
	[KEY_N] = "KEY_N",
	[KEY_M] = "KEY_M",
	[KEY_COMMA] = "KEY_COMMA",
	[KEY_DOT] = "KEY_DOT",
	[KEY_SLASH] = "KEY_SLASH",
	[KEY_RIGHTSHIFT] = "KEY_RIGHTSHIFT",
	[KEY_KPASTERISK] = "KEY_KPASTERISK",
	[KEY_LEFTALT] = "KEY_LEFTALT",
	[KEY_SPACE] = "KEY_SPACE",
	[KEY_CAPSLOCK] = "KEY_CAPSLOCK",
	[KEY_F1] = "KEY_F1",
	[KEY_F2] = "KEY_F2",
	[KEY_F3] = "KEY_F3",
	[KEY_F4] = "KEY_F4",
	[KEY_F5] = "KEY_F5",
	[KEY_F6] = "KEY_F6",
	[KEY_F7] = "KEY_F7",
	[KEY_F8] = "KEY_F8",
	[KEY_F9] = "KEY_F9",
	[KEY_F10] = "KEY_F10",
	[KEY_NUMLOCK] = "KEY_NUMLOCK",
	[KEY_SCROLLLOCK] = "KEY_SCROLLLOCK",
	[KEY_KP7] = "KEY_KP7",
	[KEY_KP8] = "KEY_KP8",
	[KEY_KP9] = "KEY_KP9",
	[KEY_KPMINUS] = "KEY_KPMINUS",
	[KEY_KP4] = "KEY_KP4",
	[KEY_KP5] = "KEY_KP5",
	[KEY_KP6] = "KEY_KP6",
	[KEY_KPPLUS] = "KEY_KPPLUS",
	[KEY_KP1] = "KEY_KP1",
	[KEY_KP2] = "KEY_KP2",
	[KEY_KP3] = "KEY_KP3",
	[KEY_KP0] = "KEY_KP0",
	[KEY_KPDOT] = "KEY_KPDOT",

	[KEY_ZENKAKUHANKAKU] = "KEY_ZENKAKUHANKAKU",
	[KEY_102ND] = "KEY_102ND",
	[KEY_F11] = "KEY_F11",
	[KEY_F12] = "KEY_F12",
	[KEY_RO] = "KEY_RO",
	[KEY_KATAKANA] = "KEY_KATAKANA",
	[KEY_HIRAGANA] = "KEY_HIRAGANA",
	[KEY_HENKAN] = "KEY_HENKAN",
	[KEY_KATAKANAHIRAGANA] = "KEY_KATAKANAHIRAGANA",
	[KEY_MUHENKAN] = "KEY_MUHENKAN",
	[KEY_KPJPCOMMA] = "KEY_KPJPCOMMA",
	[KEY_KPENTER] = "KEY_KPENTER",
	[KEY_RIGHTCTRL] = "KEY_RIGHTCTRL",
	[KEY_KPSLASH] = "KEY_KPSLASH",
	[KEY_SYSRQ] = "KEY_SYSRQ",
	[KEY_RIGHTALT] = "KEY_RIGHTALT",
	[KEY_LINEFEED] = "KEY_LINEFEED",
	[KEY_HOME] = "KEY_HOME",
	[KEY_UP] = "KEY_UP",
	[KEY_PAGEUP] = "KEY_PAGEUP",
	[KEY_LEFT] = "KEY_LEFT",
	[KEY_RIGHT] = "KEY_RIGHT",
	[KEY_END] = "KEY_END",
	[KEY_DOWN] = "KEY_DOWN",
	[KEY_PAGEDOWN] = "KEY_PAGEDOWN",
	[KEY_INSERT] = "KEY_INSERT",
	[KEY_DELETE] = "KEY_DELETE",
	[KEY_MACRO] = "KEY_MACRO",
	[KEY_MUTE] = "KEY_MUTE",
	[KEY_VOLUMEDOWN] = "KEY_VOLUMEDOWN",
	[KEY_VOLUMEUP] = "KEY_VOLUMEUP",
	[KEY_POWER] = "KEY_POWER",
	[KEY_KPEQUAL] = "KEY_KPEQUAL",
	[KEY_KPPLUSMINUS] = "KEY_KPPLUSMINUS",
	[KEY_PAUSE] = "KEY_PAUSE",
	[KEY_SCALE] = "KEY_SCALE",

	[KEY_KPCOMMA] = "KEY_KPCOMMA",
	[KEY_HANGEUL] = "KEY_HANGEUL",
	[KEY_HANGUEL] = "KEY_HANGUEL",
	[KEY_HANJA] = "KEY_HANJA",
	[KEY_YEN] = "KEY_YEN",
	[KEY_LEFTMETA] = "KEY_LEFTMETA",
	[KEY_RIGHTMETA] = "KEY_RIGHTMETA",
	[KEY_COMPOSE] = "KEY_COMPOSE",

	[KEY_STOP] = "KEY_STOP",
	[KEY_AGAIN] = "KEY_AGAIN",
	[KEY_PROPS] = "KEY_PROPS",
	[KEY_UNDO] = "KEY_UNDO",
	[KEY_FRONT] = "KEY_FRONT",
	[KEY_COPY] = "KEY_COPY",
	[KEY_OPEN] = "KEY_OPEN",
	[KEY_PASTE] = "KEY_PASTE",
	[KEY_FIND] = "KEY_FIND",
	[KEY_CUT] = "KEY_CUT",
	[KEY_HELP] = "KEY_HELP",
	[KEY_MENU] = "KEY_MENU",
	[KEY_CALC] = "KEY_CALC",
	[KEY_SETUP] = "KEY_SETUP",
	[KEY_SLEEP] = "KEY_SLEEP",
	[KEY_WAKEUP] = "KEY_WAKEUP",
	[KEY_FILE] = "KEY_FILE",
	[KEY_SENDFILE] = "KEY_SENDFILE",
	[KEY_DELETEFILE] = "KEY_DELETEFILE",
	[KEY_XFER] = "KEY_XFER",
	[KEY_PROG1] = "KEY_PROG1",
	[KEY_PROG2] = "KEY_PROG2",
	[KEY_WWW] = "KEY_WWW",
	[KEY_MSDOS] = "KEY_MSDOS",
	[KEY_COFFEE] = "KEY_COFFEE",
	[KEY_SCREENLOCK] = "KEY_SCREENLOCK",
	[KEY_DIRECTION] = "KEY_DIRECTION",
	[KEY_CYCLEWINDOWS] = "KEY_CYCLEWINDOWS",
	[KEY_MAIL] = "KEY_MAIL",
	[KEY_BOOKMARKS] = "KEY_BOOKMARKS",
	[KEY_COMPUTER] = "KEY_COMPUTER",
	[KEY_BACK] = "KEY_BACK",
	[KEY_FORWARD] = "KEY_FORWARD",
	[KEY_CLOSECD] = "KEY_CLOSECD",
	[KEY_EJECTCD] = "KEY_EJECTCD",
	[KEY_EJECTCLOSECD] = "KEY_EJECTCLOSECD",
	[KEY_NEXTSONG] = "KEY_NEXTSONG",
	[KEY_PLAYPAUSE] = "KEY_PLAYPAUSE",
	[KEY_PREVIOUSSONG] = "KEY_PREVIOUSSONG",
	[KEY_STOPCD] = "KEY_STOPCD",
	[KEY_RECORD] = "KEY_RECORD",
	[KEY_REWIND] = "KEY_REWIND",
	[KEY_PHONE] = "KEY_PHONE",
	[KEY_ISO] = "KEY_ISO",
	[KEY_CONFIG] = "KEY_CONFIG",
	[KEY_HOMEPAGE] = "KEY_HOMEPAGE",
	[KEY_REFRESH] = "KEY_REFRESH",
	[KEY_EXIT] = "KEY_EXIT",
	[KEY_MOVE] = "KEY_MOVE",
	[KEY_EDIT] = "KEY_EDIT",
	[KEY_SCROLLUP] = "KEY_SCROLLUP",
	[KEY_SCROLLDOWN] = "KEY_SCROLLDOWN",
	[KEY_KPLEFTPAREN] = "KEY_KPLEFTPAREN",
	[KEY_KPRIGHTPAREN] = "KEY_KPRIGHTPAREN",
	[KEY_NEW] = "KEY_NEW",
	[KEY_REDO] = "KEY_REDO",

	[KEY_F13] = "KEY_F13",
	[KEY_F14] = "KEY_F14",
	[KEY_F15] = "KEY_F15",
	[KEY_F16] = "KEY_F16",
	[KEY_F17] = "KEY_F17",
	[KEY_F18] = "KEY_F18",
	[KEY_F19] = "KEY_F19",
	[KEY_F20] = "KEY_F20",
	[KEY_F21] = "KEY_F21",
	[KEY_F22] = "KEY_F22",
	[KEY_F23] = "KEY_F23",
	[KEY_F24] = "KEY_F24",

	[KEY_PLAYCD] = "KEY_PLAYCD",
	[KEY_PAUSECD] = "KEY_PAUSECD",
	[KEY_PROG3] = "KEY_PROG3",
	[KEY_PROG4] = "KEY_PROG4",
	[KEY_DASHBOARD] = "KEY_DASHBOARD",
	[KEY_SUSPEND] = "KEY_SUSPEND",
	[KEY_CLOSE] = "KEY_CLOSE",
	[KEY_PLAY] = "KEY_PLAY",
	[KEY_FASTFORWARD] = "KEY_FASTFORWARD",
	[KEY_BASSBOOST] = "KEY_BASSBOOST",
	[KEY_PRINT] = "KEY_PRINT",
	[KEY_HP] = "KEY_HP",
	[KEY_CAMERA] = "KEY_CAMERA",
	[KEY_SOUND] = "KEY_SOUND",
	[KEY_QUESTION] = "KEY_QUESTION",
	[KEY_EMAIL] = "KEY_EMAIL",
	[KEY_CHAT] = "KEY_CHAT",
	[KEY_SEARCH] = "KEY_SEARCH",
	[KEY_CONNECT] = "KEY_CONNECT",
	[KEY_FINANCE] = "KEY_FINANCE",
	[KEY_SPORT] = "KEY_SPORT",
	[KEY_SHOP] = "KEY_SHOP",
	[KEY_ALTERASE] = "KEY_ALTERASE",
	[KEY_CANCEL] = "KEY_CANCEL",
	[KEY_BRIGHTNESSDOWN] = "KEY_BRIGHTNESSDOWN",
	[KEY_BRIGHTNESSUP] = "KEY_BRIGHTNESSUP",
	[KEY_MEDIA] = "KEY_MEDIA",

	[KEY_SWITCHVIDEOMODE] = "KEY_SWITCHVIDEOMODE",
	[KEY_KBDILLUMTOGGLE] = "KEY_KBDILLUMTOGGLE",
	[KEY_KBDILLUMDOWN] = "KEY_KBDILLUMDOWN",
	[KEY_KBDILLUMUP] = "KEY_KBDILLUMUP",

	[KEY_SEND] = "KEY_SEND",
	[KEY_REPLY] = "KEY_REPLY",
	[KEY_FORWARDMAIL] = "KEY_FORWARDMAIL",
	[KEY_SAVE] = "KEY_SAVE",
	[KEY_DOCUMENTS] = "KEY_DOCUMENTS",

	[KEY_BATTERY] = "KEY_BATTERY",

	[KEY_BLUETOOTH] = "KEY_BLUETOOTH",
	[KEY_WLAN] = "KEY_WLAN",
	[KEY_UWB] = "KEY_UWB",

	[KEY_UNKNOWN] = "KEY_UNKNOWN",

	[KEY_VIDEO_NEXT] = "KEY_VIDEO_NEXT",
	[KEY_VIDEO_PREV] = "KEY_VIDEO_PREV",
	[KEY_BRIGHTNESS_CYCLE] = "KEY_BRIGHTNESS_CYCLE",
	[KEY_BRIGHTNESS_ZERO] = "KEY_BRIGHTNESS_ZERO",
	[KEY_DISPLAY_OFF] = "KEY_DISPLAY_OFF",

	[KEY_WIMAX] = "KEY_WIMAX",

	[BTN_0] = "BTN_0",
	[BTN_1] = "BTN_1",

	[BTN_2] = "BTN_2",
	[BTN_3] = "BTN_3",

	[BTN_4] = "BTN_4",
	[BTN_5] = "BTN_5",

	[BTN_6] = "BTN_6",
	[BTN_7] = "BTN_7",

	[BTN_8] = "BTN_8",
	[BTN_9] = "BTN_9",

	[BTN_LEFT] = "BTN_LEFT",
	[BTN_RIGHT] = "BTN_RIGHT",

	[BTN_MIDDLE] = "BTN_MIDDLE",
	[BTN_SIDE] = "BTN_SIDE",

	[BTN_EXTRA] = "BTN_EXTRA",
	[BTN_FORWARD] = "BTN_FORWARD",

	[BTN_BACK] = "BTN_BACK",
	[BTN_TASK] = "BTN_TASK",

	[BTN_TRIGGER] = "BTN_TRIGGER",
	[BTN_THUMB] = "BTN_THUMB",

	[BTN_THUMB2] = "BTN_THUMB2",
	[BTN_TOP] = "BTN_TOP",

	[BTN_TOP2] = "BTN_TOP2",
	[BTN_PINKIE] = "BTN_PINKIE",

	[BTN_BASE] = "BTN_BASE",
	[BTN_BASE2] = "BTN_BASE2",

	[BTN_BASE3] = "BTN_BASE3",
	[BTN_BASE4] = "BTN_BASE4",

	[BTN_BASE5] = "BTN_BASE5",
	[BTN_BASE6] = "BTN_BASE6",

	[BTN_DEAD] = "BTN_DEAD",
	[BTN_A] = "BTN_A",

	[BTN_B] = "BTN_B",
	[BTN_C] = "BTN_C",

	[BTN_X] = "BTN_X",
	[BTN_Y] = "BTN_Y",

	[BTN_Z] = "BTN_Z",
	[BTN_TL] = "BTN_TL",

	[BTN_TR] = "BTN_TR",
	[BTN_TL2] = "BTN_TL2",

	[BTN_TR2] = "BTN_TR2",
	[BTN_SELECT] = "BTN_SELECT",

	[BTN_START] = "BTN_START",
	[BTN_MODE] = "BTN_MODE",

	[BTN_THUMBL] = "BTN_THUMBL",
	[BTN_THUMBR] = "BTN_THUMBR",

	[BTN_TOOL_PEN] = "BTN_TOOL_PEN",
	[BTN_TOOL_RUBBER] = "BTN_TOOL_RUBBER",

	[BTN_TOOL_BRUSH] = "BTN_TOOL_BRUSH",
	[BTN_TOOL_PENCIL] = "BTN_TOOL_PENCIL",

	[BTN_TOOL_AIRBRUSH] = "BTN_TOOL_AIRBRUSH",
	[BTN_TOOL_FINGER] = "BTN_TOOL_FINGER",

	[BTN_TOOL_MOUSE] = "BTN_TOOL_MOUSE",
	[BTN_TOOL_LENS] = "BTN_TOOL_LENS",

	[BTN_TOUCH] = "BTN_TOUCH",
	[BTN_STYLUS] = "BTN_STYLUS",

	[BTN_STYLUS2] = "BTN_STYLUS2",
	[BTN_TOOL_DOUBLETAP] = "BTN_TOOL_DOUBLETAP",

	[BTN_TOOL_TRIPLETAP] = "BTN_TOOL_TRIPLETAP",
	[BTN_GEAR_DOWN] = "BTN_GEAR_DOWN",

	[BTN_GEAR_UP] = "BTN_GEAR_UP",
	[KEY_OK] = "KEY_OK",

	[KEY_SELECT] = "KEY_SELECT",
	[KEY_GOTO] = "KEY_GOTO",

	[KEY_CLEAR] = "KEY_CLEAR",
	[KEY_POWER2] = "KEY_POWER2",

	[KEY_OPTION] = "KEY_OPTION",
	[KEY_INFO] = "KEY_INFO",

	[KEY_TIME] = "KEY_TIME",
	[KEY_VENDOR] = "KEY_VENDOR",

	[KEY_ARCHIVE] = "KEY_ARCHIVE",
	[KEY_PROGRAM] = "KEY_PROGRAM",

	[KEY_CHANNEL] = "KEY_CHANNEL",
	[KEY_FAVORITES] = "KEY_FAVORITES",

	[KEY_EPG] = "KEY_EPG",
	[KEY_PVR] = "KEY_PVR",

	[KEY_MHP] = "KEY_MHP",
	[KEY_LANGUAGE] = "KEY_LANGUAGE",

	[KEY_TITLE] = "KEY_TITLE",
	[KEY_SUBTITLE] = "KEY_SUBTITLE",

	[KEY_ANGLE] = "KEY_ANGLE",
	[KEY_ZOOM] = "KEY_ZOOM",

	[KEY_MODE] = "KEY_MODE",
	[KEY_KEYBOARD] = "KEY_KEYBOARD",

	[KEY_SCREEN] = "KEY_SCREEN",
	[KEY_PC] = "KEY_PC",

	[KEY_TV] = "KEY_TV",
	[KEY_TV2] = "KEY_TV2",

	[KEY_VCR] = "KEY_VCR",
	[KEY_VCR2] = "KEY_VCR2",

	[KEY_SAT] = "KEY_SAT",
	[KEY_SAT2] = "KEY_SAT2",

	[KEY_CD] = "KEY_CD",
	[KEY_TAPE] = "KEY_TAPE",

	[KEY_RADIO] = "KEY_RADIO",
	[KEY_TUNER] = "KEY_TUNER",

	[KEY_PLAYER] = "KEY_PLAYER",
	[KEY_TEXT] = "KEY_TEXT",

	[KEY_DVD] = "KEY_DVD",
	[KEY_AUX] = "KEY_AUX",

	[KEY_MP3] = "KEY_MP3",
	[KEY_AUDIO] = "KEY_AUDIO",

	[KEY_VIDEO] = "KEY_VIDEO",
	[KEY_DIRECTORY] = "KEY_DIRECTORY",

	[KEY_LIST] = "KEY_LIST",
	[KEY_MEMO] = "KEY_MEMO",

	[KEY_CALENDAR] = "KEY_CALENDAR",
	[KEY_RED] = "KEY_RED",

	[KEY_GREEN] = "KEY_GREEN",
	[KEY_YELLOW] = "KEY_YELLOW",

	[KEY_BLUE] = "KEY_BLUE",
	[KEY_CHANNELUP] = "KEY_CHANNELUP",

	[KEY_CHANNELDOWN] = "KEY_CHANNELDOWN",
	[KEY_FIRST] = "KEY_FIRST",

	[KEY_LAST] = "KEY_LAST",
	[KEY_AB] = "KEY_AB",

	[KEY_NEXT] = "KEY_NEXT",
	[KEY_RESTART] = "KEY_RESTART",

	[KEY_SLOW] = "KEY_SLOW",
	[KEY_SHUFFLE] = "KEY_SHUFFLE",

	[KEY_BREAK] = "KEY_BREAK",
	[KEY_PREVIOUS] = "KEY_PREVIOUS",

	[KEY_DIGITS] = "KEY_DIGITS",
	[KEY_TEEN] = "KEY_TEEN",

	[KEY_TWEN] = "KEY_TWEN",
	[KEY_DEL_EOL] = "KEY_DEL_EOL",

	[KEY_DEL_EOS] = "KEY_DEL_EOS",
	[KEY_INS_LINE] = "KEY_INS_LINE",

	[KEY_DEL_LINE] = "KEY_DEL_LINE",
};

const char *events[EV_MAX + 1] = {
	[0 ... EV_MAX] = NULL,
	[EV_SYN] = "EV_SYN",
	[EV_KEY] = "EV_KEY",
	[EV_REL] = "EV_REL",
	[EV_ABS] = "EV_ABS",
	[EV_MSC] = "EV_MSC",
	[EV_SW]  = "EV_SW",
	[EV_LED] = "EV_LED",
	[EV_SND] = "EV_SND",
	[EV_REP] = "EV_REP",
	[EV_FF] = "EV_FF",
	[EV_PWR] = "EV_PWR",
	[EV_FF_STATUS] = "EV_FF_STATUS",
};

const char *relatives[REL_MAX + 1] = {
	[0 ... REL_MAX] = NULL,
	[REL_X] = "REL_X",
	[REL_Y] = "REL_Y",
	[REL_Z] = "REL_Z",
	[REL_HWHEEL] = "REL_HWHEEL",
	[REL_DIAL] = "REL_DIAL",
	[REL_WHEEL] = "REL_WHEEL",
	[REL_MISC] = "REL_MISC",
};

const char *absolutes[ABS_MAX + 1] = {
	[0 ... ABS_MAX] = NULL,
	[ABS_X] = "ABS_X",
	[ABS_Y] = "ABS_Y",
	[ABS_Z] = "ABS_Z",
	[ABS_RX] = "ABS_RX",
	[ABS_RY] = "ABS_RY",
	[ABS_RZ] = "ABS_RZ",
	[ABS_THROTTLE] = "ABS_THROTTLE",
	[ABS_RUDDER] = "ABS_RUDDER",
	[ABS_WHEEL] = "ABS_WHEEL",
	[ABS_GAS] = "ABS_GAS",
	[ABS_BRAKE] = "ABS_BRAKE",
	[ABS_HAT0X] = "ABS_HAT0X",
	[ABS_HAT0Y] = "ABS_HAT0Y",
	[ABS_HAT1X] = "ABS_HAT1X",
	[ABS_HAT1Y] = "ABS_HAT1Y",
	[ABS_HAT2X] = "ABS_HAT2X",
	[ABS_HAT2Y] = "ABS_HAT2Y",
	[ABS_HAT3X] = "ABS_HAT3X",
	[ABS_HAT3Y] = "ABS_HAT3Y",
	[ABS_PRESSURE] = "ABS_PRESSURE",
	[ABS_DISTANCE] = "ABS_DISTANCE",
	[ABS_TILT_X] = "ABS_TILT_X",
	[ABS_TILT_Y] = "ABS_TILT_Y",
	[ABS_TOOL_WIDTH] = "ABS_TOOL_WIDTH",
	[ABS_VOLUME] = "ABS_VOLUME",
	[ABS_MISC] = "ABS_MISC",
	[ABS_MT_TOUCH_MAJOR] = "ABS_MT_TOUCH_MAJOR",
	[ABS_MT_TOUCH_MINOR] = "ABS_MT_TOUCH_MINOR",
	[ABS_MT_WIDTH_MAJOR] = "ABS_MT_WIDTH_MAJOR",
	[ABS_MT_WIDTH_MINOR] = "ABS_MT_WIDTH_MINOR",
	[ABS_MT_ORIENTATION] = "ABS_MT_ORIENTATION",
	[ABS_MT_POSITION_X] = "ABS_MT_POSITION_X",
	[ABS_MT_POSITION_Y] = "ABS_MT_POSITION_Y",
	[ABS_MT_TOOL_TYPE] = "ABS_MT_TOOL_TYPE",
	[ABS_MT_BLOB_ID] = "ABS_MT_BLOB_ID",
	[ABS_MT_TRACKING_ID] = "ABS_MT_TRACKING_ID",
};

const char *misc[MSC_MAX + 1] = {
	[ 0 ... MSC_MAX] = NULL,
	[MSC_SERIAL] = "MSC_SERIAL",
	[MSC_PULSELED] = "MSC_PULSELED",
	[MSC_GESTURE] = "MSC_GESTURE",
	[MSC_RAW] = "MSC_RAW",
	[MSC_SCAN] = "MSC_SCAN",
};

const char *leds[LED_MAX + 1] = {
	[0 ... LED_MAX] = NULL,
	[LED_NUML] = "LED_NUML",
	[LED_CAPSL] = "LED_CAPSL",
	[LED_SCROLLL] = "LED_SCROLLL",
	[LED_COMPOSE] = "LED_COMPOSE",
	[LED_KANA] = "LED_KANA",
	[LED_SLEEP] = "LED_SLEEP",
	[LED_SUSPEND] = "LED_SUSPEND",
	[LED_MUTE] = "LED_MUTE",
	[LED_MISC] = "LED_MISC",

};

const char *repeats[REP_MAX + 1] = {
	[0 ... REP_MAX] = NULL,
	[REP_DELAY] = "REP_DELAY",
	[REP_PERIOD] = "REP_PERIOD"
};

const char *sounds[SND_MAX + 1] = {
	[0 ... SND_MAX] = NULL,
	[SND_CLICK] = "SND_CLICK",
	[SND_BELL] = "SND_BELL",
	[SND_TONE] = "SND_TONE"
};

const char **names[EV_MAX + 1] = {
	[0 ... EV_MAX] = NULL,
	[EV_SYN] = events,
	[EV_KEY] = keys,
	[EV_REL] = relatives,
	[EV_ABS] = absolutes,
	[EV_MSC] = misc,
	[EV_LED] = leds,
	[EV_SND] = sounds,
	[EV_REP] = repeats,
};

char *absval[6] = { "abs-value",
		"abs-min",
		"abs-max",
		"abs-fuzz",
		"abs-flat",
		"abs-resolution"
};

static int print_capabilities(int fd, xmlTextWriterPtr writer)
{

	int rc, i;
	unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
	unsigned int abs[6];
	char buffer[64];

	memset(bit, 0, sizeof(bit));
	call(ioctl(fd, EVIOCGBIT(0, EV_MAX), bit[0]));

	for (i = 0; i < EV_MAX; i++)
	{
		if (test_bit(i, bit[0]))
		{
			int j;
			start(BAD_CAST "event-type");
			sprintf(buffer, "%s", events[i]);
			attr(BAD_CAST "type", BAD_CAST  buffer);
			if (!i)
			{
				end();
				continue;
			}
			ioctl(fd, EVIOCGBIT(i, KEY_MAX), bit[i]);
			for (j = 0; j < KEY_MAX; j++)
			{
				if (test_bit(j, bit[i]))
				{
					start(BAD_CAST "code");
					sprintf(buffer, "%s", (names[i] && names[i][j]) ?  names[i][j] : "?");
					attr(BAD_CAST "value", BAD_CAST
							buffer)

					if (i == EV_ABS) {
						int k;
						ioctl(fd, EVIOCGABS(j), abs);
						for (k = 0; k < sizeof(abs)/sizeof(abs[0]); k++)
						{
							sprintf(buffer, "%d", abs[k]);
							attr(BAD_CAST
									absval[k],
									BAD_CAST
									buffer);
						}
								/*format(BAD_CAST
								 * absval[k],
								 * "%d",
								 * abs[k]);*/
					}
					end("code");
				}
			}
			end("event-type");
		}
	}

	return 0;
error:
	return -1;
}

static int print_info(int fd, xmlTextWriterPtr writer)
{
	unsigned short id[4];
	char name[256] = "---";
	char buffer[64];
	int rc;

	call(ioctl(fd, EVIOCGID, id));
	call(ioctl(fd, EVIOCGNAME(sizeof(name)), name));

	start(BAD_CAST "info");
	start(BAD_CAST "id");
	sprintf(buffer, "%#x", id[ID_BUS]);
	attr(BAD_CAST "bus", BAD_CAST buffer);
	sprintf(buffer, "%#x", id[ID_VENDOR]);
	attr(BAD_CAST "vendor", BAD_CAST buffer);
	sprintf(buffer, "%#x", id[ID_PRODUCT]);
	attr(BAD_CAST "product", BAD_CAST buffer);
	sprintf(buffer, "%#x", id[ID_VERSION]);
	attr(BAD_CAST "version", BAD_CAST buffer);
	xmlTextWriterWriteFormatString(writer, "%s", name);
	end("id");
	call(print_capabilities(fd, writer));
	end("info");

	return 0;
error:
	return -1;
}

static int init_xml(xmlTextWriterPtr *writer)
{
	LIBXML_TEST_VERSION

	*writer = xmlNewTextWriterFilename("evtest-capture.xml", 0);
	if (!*writer)
		return -1;

	xmlTextWriterSetIndent(*writer, 1);
	xmlTextWriterSetIndentString(*writer, BAD_CAST "    ");
	return xmlTextWriterStartDocument(*writer, NULL, MY_ENCODING, NULL);
}

static void fini_xml(xmlTextWriterPtr writer)
{
	xmlTextWriterEndDocument(writer);
	xmlCleanupParser();
}

static void sig_handler(int signal)
{
	stop = 1;
	return;
}

static int print_events(int fd, xmlTextWriterPtr writer)
{
	int rc;
	struct input_event ev;
	start(BAD_CAST "events");
	char buffer[64];

	struct sigaction sigact = {
		.sa_handler = sig_handler,
	};

	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, &sigact);

	while(!stop)
	{
		rc = read(fd, &ev, sizeof(struct input_event));
		if (rc < sizeof(ev))
			goto error;

		start(BAD_CAST "event");
		sprintf(buffer, "%ld", ev.time.tv_sec);
		attr(BAD_CAST "sec", BAD_CAST buffer);
		sprintf(buffer, "%ld", ev.time.tv_usec);
		attr(BAD_CAST "usec", BAD_CAST buffer);
		sprintf(buffer, "%s", (events[ev.type]) ?  events[ev.type] : "?");
		attr(BAD_CAST "type", BAD_CAST buffer);
		sprintf(buffer, "%s", (names[ev.type] && names[ev.type][ev.code]) ?  names[ev.type][ev.code] : "?");
		attr(BAD_CAST "code", BAD_CAST buffer);
		sprintf(buffer, "%d", ev.value);
		attr(BAD_CAST "value", BAD_CAST buffer);
		end("event");
	}

	sigaction(SIGINT, &sigact, &sigact);

	end("events");
	return 0;
error:
	sigaction(SIGINT, &sigact, &sigact);
	perror("Read error: \n");
	return -1;
}

int main(int argc, char **argv)
{
	int fd = 0;
	int rc;

	xmlTextWriterPtr writer;

	if (argc < 2) {
		printf("Usage: %s /dev/input/eventX\n", argv[0]);
		printf("Where X = input device number.\n");
		return 1;
	}

	if ((fd = open(argv[argc - 1], O_RDONLY)) < 0)
		goto error;

	init_xml(&writer);

	start(BAD_CAST "evtest-capture");

	call(print_info(fd, writer));
	call(print_events(fd, writer));

	end("evtest-capture");
	fini_xml(writer);
	return 0;

error:
	xmlTextWriterEndDocument(writer);
	xmlCleanupParser();
	perror("Failed with error:");
	return 1;
}

/* vim: set noexpandtab shiftwidth=8: */
