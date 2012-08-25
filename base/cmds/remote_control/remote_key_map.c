/** @file remote_key_map.c
 *  @par Copyright:
 *  - Copyright 2011 Amlogic Inc as unpublished work                             
 *  All Rights Reserved                                                                                                                              
 *  - The information contained herein is the confidential property            
 *  of Amlogic.  The use, copying, transfer or disclosure of such information
 *  is prohibited except by express written agreement with Amlogic Inc. 
 *  @author   tellen
 *  @version  1.0        
 *  @date     2011/06/07
 *  @par function description:
 *  - 1 remote control server, map protocol key to system keycode
 *  @warning This class may explode in your face.
 *  @note If you inherit anything from this class, you're doomed.
 */


//this must sync., with KeyEvent.java
#define KEYCODE_UNKNOWN          		0
#define KEYCODE_SOFT_LEFT        		1
#define KEYCODE_SOFT_RIGHT       		2
#define KEYCODE_HOME             		3
#define KEYCODE_BACK             		4
#define KEYCODE_CALL             		5
#define KEYCODE_ENDCALL          		6
#define KEYCODE_0                		7
#define KEYCODE_1                		8
#define KEYCODE_2                		9
#define KEYCODE_3               		10
#define KEYCODE_4                		11
#define KEYCODE_5                		12
#define KEYCODE_6                		13
#define KEYCODE_7                		14
#define KEYCODE_8                		15
#define KEYCODE_9                		16
#define KEYCODE_STAR             		17
#define KEYCODE_POUND            		18
#define KEYCODE_DPAD_UP          		19
#define KEYCODE_DPAD_DOWN       		20
#define KEYCODE_DPAD_LEFT        		21
#define KEYCODE_DPAD_RIGHT       		22
#define KEYCODE_DPAD_CENTER      		23
#define KEYCODE_VOLUME_UP        		24
#define KEYCODE_VOLUME_DOWN      		25
#define KEYCODE_POWER            		26
#define KEYCODE_CAMERA           		27
#define KEYCODE_CLEAR            		28
#define KEYCODE_A                		29
#define KEYCODE_B                		30
#define KEYCODE_C                		31
#define KEYCODE_D                		32
#define KEYCODE_E                		33
#define KEYCODE_F                		34
#define KEYCODE_G                		35
#define KEYCODE_H                		36
#define KEYCODE_I                		37
#define KEYCODE_J                		38
#define KEYCODE_K                		39
#define KEYCODE_L                		40
#define KEYCODE_M                		41
#define KEYCODE_N                		42
#define KEYCODE_O                		43
#define KEYCODE_P                		44
#define KEYCODE_Q                		45
#define KEYCODE_R                		46
#define KEYCODE_S                		47
#define KEYCODE_T                		48
#define KEYCODE_U                		49
#define KEYCODE_V                		50
#define KEYCODE_W                		51
#define KEYCODE_X                		52
#define KEYCODE_Y                		53
#define KEYCODE_Z                		54
#define KEYCODE_COMMA            		55
#define KEYCODE_PERIOD           		56
#define KEYCODE_ALT_LEFT         		57
#define KEYCODE_ALT_RIGHT        		58
#define KEYCODE_SHIFT_LEFT       		59
#define KEYCODE_SHIFT_RIGHT      		60
#define KEYCODE_TAB              		61
#define KEYCODE_SPACE            		62
#define KEYCODE_SYM              		63
#define KEYCODE_EXPLORER         		64
#define KEYCODE_ENVELOPE         		65
#define KEYCODE_ENTER            		66
#define KEYCODE_DEL              		67
#define KEYCODE_GRAVE            		68
#define KEYCODE_MINUS            		69
#define KEYCODE_EQUALS           		70
#define KEYCODE_LEFT_BRACKET     		71
#define KEYCODE_RIGHT_BRACKET    		72
#define KEYCODE_BACKSLASH        		73
#define KEYCODE_SEMICOLON        		74
#define KEYCODE_APOSTROPHE       		75
#define KEYCODE_SLASH            		76
#define KEYCODE_AT               		77
#define KEYCODE_NUM              		78
#define KEYCODE_HEADSETHOOK      		79
#define KEYCODE_FOCUS            		80   // *Camera* focus
#define KEYCODE_PLUS             		81
#define KEYCODE_MENU             		82
#define KEYCODE_NOTIFICATION     		83
#define KEYCODE_SEARCH           		84
#define KEYCODE_MEDIA_PLAY_PAUSE		85
#define KEYCODE_MEDIA_STOP       		86
#define KEYCODE_MEDIA_NEXT       		87
#define KEYCODE_MEDIA_PREVIOUS			88
#define KEYCODE_MEDIA_REWIND     		89
#define KEYCODE_MEDIA_FAST_FORWARD		90
#define KEYCODE_MUTE             		91

//define protocol keycode
#define RC_KEYCODE_HOME					0
#define RC_KEYCODE_MENU					1
#define RC_KEYCODE_BACK					2
#define RC_KEYCODE_VOLUME_UP			3
#define RC_KEYCODE_VOLUME_DOWN			4
#define RC_KEYCODE_SEARCH				5
#define RC_KEYCODE_MUTE					6
#define RC_KEYCODE_UP					7
#define RC_KEYCODE_DOWN					8
#define RC_KEYCODE_LEFT					9
#define RC_KEYCODE_RIGHT				10
#define RC_KEYCODE_CENTER				11
#define RC_KEYCODE_POWER				12
#define RC_KEYCODE_CAMERA				13
#define RC_KEYCODE_CLEAR				14
#define RC_KEYCODE_SOFT_LEFT			15
#define RC_KEYCODE_SOFT_RIGHT			16
#define RC_KEYCODE_CALL					17
#define RC_KEYCODE_ENDCALL				18
#define RC_KEYCODE_0					19
#define RC_KEYCODE_1					20
#define RC_KEYCODE_2					21
#define RC_KEYCODE_3					22
#define RC_KEYCODE_4					23
#define RC_KEYCODE_5					24
#define RC_KEYCODE_6					25
#define RC_KEYCODE_7					26
#define RC_KEYCODE_8					27
#define RC_KEYCODE_9					28
#define RC_KEYCODE_STAR					29
#define RC_KEYCODE_POUND				30
#define RC_KEYCODE_A					31
#define RC_KEYCODE_B                	32
#define RC_KEYCODE_C                	33
#define RC_KEYCODE_D                	34
#define RC_KEYCODE_E                	35
#define RC_KEYCODE_F                	36
#define RC_KEYCODE_G                	37
#define RC_KEYCODE_H                	38
#define RC_KEYCODE_I                	39
#define RC_KEYCODE_J                	40
#define RC_KEYCODE_K                	41
#define RC_KEYCODE_L                	42
#define RC_KEYCODE_M                	43
#define RC_KEYCODE_N                	44
#define RC_KEYCODE_O                	45
#define RC_KEYCODE_P                	46
#define RC_KEYCODE_Q                	47
#define RC_KEYCODE_R                	48
#define RC_KEYCODE_S                	49
#define RC_KEYCODE_T                	50
#define RC_KEYCODE_U                	51
#define RC_KEYCODE_V                	52
#define RC_KEYCODE_W                	53
#define RC_KEYCODE_X                	54
#define RC_KEYCODE_Y                	55
#define RC_KEYCODE_Z               		56
#define RC_KEYCODE_COMMA            	57
#define RC_KEYCODE_PERIOD           	58
#define RC_KEYCODE_ALT_LEFT         	59
#define RC_KEYCODE_ALT_RIGHT        	60
#define RC_KEYCODE_SHIFT_LEFT       	61
#define RC_KEYCODE_SHIFT_RIGHT      	62
#define RC_KEYCODE_TAB              	63
#define RC_KEYCODE_SPACE            	64
#define RC_KEYCODE_SYM              	65
#define RC_KEYCODE_EXPLORER         	66
#define RC_KEYCODE_ENVELOPE         	67
#define RC_KEYCODE_ENTER            	68
#define RC_KEYCODE_DEL              	69
#define RC_KEYCODE_GRAVE            	70
#define RC_KEYCODE_MINUS            	71
#define RC_KEYCODE_EQUALS           	72
#define RC_KEYCODE_LEFT_BRACKET     	73
#define RC_KEYCODE_RIGHT_BRACKET    	74
#define RC_KEYCODE_BACKSLASH        	75
#define RC_KEYCODE_SEMICOLON        	76
#define RC_KEYCODE_APOSTROPHE       	77
#define RC_KEYCODE_SLASH            	78
#define RC_KEYCODE_AT               	79
#define RC_KEYCODE_NUM              	80
#define RC_KEYCODE_HEADSETHOOK      	81
#define RC_KEYCODE_FOCUS            	82   // *Camera* focus
#define RC_KEYCODE_PLUS             	83
#define RC_KEYCODE_NOTIFICATION     	84
#define RC_KEYCODE_MEDIA_PLAY_PAUSE		85
#define RC_KEYCODE_MEDIA_STOP       	86
#define RC_KEYCODE_MEDIA_NEXT       	87
#define RC_KEYCODE_MEDIA_PREVIOUS   	88
#define RC_KEYCODE_MEDIA_REWIND     	89
#define RC_KEYCODE_MEDIA_FAST_FORWARD  90
#define RC_KEYCODE_UNKNOWN				91
#define RC_KEYCODE_TOTAL				92

#define RC_KEYCODE_PAD_MOUSE_SWITCH		200
#define RC_KEYCODE_PAD_MOUSE_SELECT		201

typedef struct
{
	unsigned char	rc_keycode;//self define keycode
	unsigned char	sys_keycode;//system keycode
}rc_key_map_t;

rc_key_map_t g_key_map[] =  
{
	{RC_KEYCODE_UNKNOWN,   		   		KEYCODE_UNKNOWN},
	{RC_KEYCODE_SOFT_LEFT,   		 	KEYCODE_SOFT_LEFT},
	{RC_KEYCODE_SOFT_RIGHT,   		 	KEYCODE_SOFT_RIGHT},
	{RC_KEYCODE_HOME,   		 		KEYCODE_HOME},
	{RC_KEYCODE_BACK,   		 		KEYCODE_BACK},
	{RC_KEYCODE_CALL,   		 		KEYCODE_CALL},
	{RC_KEYCODE_ENDCALL,   		 		KEYCODE_ENDCALL},
	{RC_KEYCODE_0,   		 			KEYCODE_0},
	{RC_KEYCODE_1,   		 			KEYCODE_1},
	{RC_KEYCODE_2,   		 			KEYCODE_2},
	{RC_KEYCODE_3,  		   			KEYCODE_3},
	{RC_KEYCODE_4,   		 			KEYCODE_4},
	{RC_KEYCODE_5,   		 			KEYCODE_5},
	{RC_KEYCODE_6,   		 			KEYCODE_6},
	{RC_KEYCODE_7,   		 			KEYCODE_7},
	{RC_KEYCODE_8,   		 			KEYCODE_8},
	{RC_KEYCODE_9,    		 			KEYCODE_9},
	{RC_KEYCODE_STAR,    		 		KEYCODE_STAR},
	{RC_KEYCODE_POUND,    		 		KEYCODE_POUND},
	{RC_KEYCODE_UP,    		 			KEYCODE_DPAD_UP},
	{RC_KEYCODE_DOWN,   		   		KEYCODE_DPAD_DOWN},
	{RC_KEYCODE_LEFT,    		 		KEYCODE_DPAD_LEFT},
	{RC_KEYCODE_RIGHT,    		 		KEYCODE_DPAD_RIGHT},
	{RC_KEYCODE_CENTER,    		 		KEYCODE_DPAD_CENTER},
	{RC_KEYCODE_VOLUME_UP,    		 	KEYCODE_VOLUME_UP},
	{RC_KEYCODE_VOLUME_DOWN,			KEYCODE_VOLUME_DOWN},
	{RC_KEYCODE_POWER,    		 		KEYCODE_POWER},
	{RC_KEYCODE_CAMERA,    		 		KEYCODE_CAMERA},
	{RC_KEYCODE_CLEAR,    		 		KEYCODE_CLEAR},
	{RC_KEYCODE_A,    		 			KEYCODE_A},
	{RC_KEYCODE_B,    		 			KEYCODE_B},
	{RC_KEYCODE_C,    		 			KEYCODE_C},
	{RC_KEYCODE_D,    		 			KEYCODE_D},
	{RC_KEYCODE_E,    		 			KEYCODE_E},
	{RC_KEYCODE_F,    		 			KEYCODE_F},
	{RC_KEYCODE_G,    		 			KEYCODE_G},
	{RC_KEYCODE_H,    		 			KEYCODE_H},
	{RC_KEYCODE_I,    		 			KEYCODE_I},
	{RC_KEYCODE_J,    		 			KEYCODE_J},
	{RC_KEYCODE_K,    		 			KEYCODE_K},
	{RC_KEYCODE_L,    		 			KEYCODE_L},
	{RC_KEYCODE_M,    		 			KEYCODE_M},
	{RC_KEYCODE_N,    		 			KEYCODE_N},
	{RC_KEYCODE_O,    		 			KEYCODE_O},
	{RC_KEYCODE_P,    		 			KEYCODE_P},
	{RC_KEYCODE_Q,    		 			KEYCODE_Q},
	{RC_KEYCODE_R,    		 			KEYCODE_R},
	{RC_KEYCODE_S,    		 			KEYCODE_S},
	{RC_KEYCODE_T,    		 			KEYCODE_T},
	{RC_KEYCODE_U,    		 			KEYCODE_U},
	{RC_KEYCODE_V,    		 			KEYCODE_V},
	{RC_KEYCODE_W,    		 			KEYCODE_W},
	{RC_KEYCODE_X,    		 			KEYCODE_X},
	{RC_KEYCODE_Y,    			 		KEYCODE_Y},
	{RC_KEYCODE_Z,    		 			KEYCODE_Z},
	{RC_KEYCODE_COMMA,    		 		KEYCODE_COMMA},
	{RC_KEYCODE_PERIOD,    		 		KEYCODE_PERIOD},
	{RC_KEYCODE_ALT_LEFT,    		 	KEYCODE_ALT_LEFT},
	{RC_KEYCODE_ALT_RIGHT,    			KEYCODE_ALT_RIGHT},
	{RC_KEYCODE_SHIFT_LEFT,    		 	KEYCODE_SHIFT_LEFT},
	{RC_KEYCODE_SHIFT_RIGHT,			KEYCODE_SHIFT_RIGHT},
	{RC_KEYCODE_TAB,    		 		KEYCODE_TAB},
	{RC_KEYCODE_SPACE,    		 		KEYCODE_SPACE},
	{RC_KEYCODE_SYM,    		 		KEYCODE_SYM},
	{RC_KEYCODE_EXPLORER,    			KEYCODE_EXPLORER},
	{RC_KEYCODE_ENVELOPE,    		 	KEYCODE_ENVELOPE},
	{RC_KEYCODE_ENTER,    		 		KEYCODE_ENTER},
	{RC_KEYCODE_DEL,    		 		KEYCODE_DEL},
	{RC_KEYCODE_GRAVE,    		 		KEYCODE_GRAVE},
	{RC_KEYCODE_MINUS,    		 		KEYCODE_MINUS},
	{RC_KEYCODE_EQUALS,    		 		KEYCODE_EQUALS},
	{RC_KEYCODE_LEFT_BRACKET,			KEYCODE_LEFT_BRACKET},
	{RC_KEYCODE_RIGHT_BRACKET,			KEYCODE_RIGHT_BRACKET},
	{RC_KEYCODE_BACKSLASH,    		 	KEYCODE_BACKSLASH},
	{RC_KEYCODE_SEMICOLON,    		 	KEYCODE_SEMICOLON},
	{RC_KEYCODE_APOSTROPHE,    		 	KEYCODE_APOSTROPHE},
	{RC_KEYCODE_SLASH,    		 		KEYCODE_SLASH},
	{RC_KEYCODE_AT,    		 			KEYCODE_AT},
	{RC_KEYCODE_NUM,    		 		KEYCODE_NUM},
	{RC_KEYCODE_HEADSETHOOK,			KEYCODE_HEADSETHOOK},
	{RC_KEYCODE_FOCUS,   		 		KEYCODE_FOCUS},
	{RC_KEYCODE_PLUS,   		 		KEYCODE_PLUS},
	{RC_KEYCODE_MENU,   		 		KEYCODE_MENU},
	{RC_KEYCODE_NOTIFICATION,			KEYCODE_NOTIFICATION},
	{RC_KEYCODE_SEARCH,   		 		KEYCODE_SEARCH},
	{RC_KEYCODE_MEDIA_PLAY_PAUSE,		KEYCODE_MEDIA_PLAY_PAUSE},
	{RC_KEYCODE_MEDIA_STOP, 		 	KEYCODE_MEDIA_STOP},
	{RC_KEYCODE_MEDIA_NEXT, 		 	KEYCODE_MEDIA_NEXT},
	{RC_KEYCODE_MEDIA_PREVIOUS,			KEYCODE_MEDIA_PREVIOUS},
	{RC_KEYCODE_MEDIA_REWIND, 		 	KEYCODE_MEDIA_REWIND},
	{RC_KEYCODE_MEDIA_FAST_FORWARD,		KEYCODE_MEDIA_FAST_FORWARD},	
	{RC_KEYCODE_MUTE,    				KEYCODE_MUTE},
};                                                  
                                                         