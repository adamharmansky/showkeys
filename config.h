
/* SETTINGS FOR SHOWKEYS */

const static int initial_width = 800;
const static int initial_height = 800;

const static char key_dir[]      = "/usr/share/showkeys"; /* directory containing the key defs (without trailing /) */

const static char color_boder[]  = "#88c0d0"; /* window border */
const static char color_bg[]     = "#3b4252"; /* background    */
const static char color_fg[]     = "#eceff4"; /* foreground    */
const static char color_title[]  = "#ffffff"; /* foreground    */
const static char color_dim_fg[] = "#929aaa"; /* modifiers     */

const static unsigned int color_count = 6;
const static char* title_colors[] = {
	"#bf616a",
	"#d08770",
	"#ebcb8b",
	"#a3be8c",
	"#5e81ac",
	"#b48ead",
};

const static char font[]          = "Ubuntu";
const static int  font_size       = 12;
const static int  title_font_size = 20;

const static int  border_padding   = 10;
const static int  title_padding    = 3;
const static int  padding_below_title = 5;
const static int  key_padding      = 2;
const static int  column_padding   = 5;

/* This is the script that checks whether to display the keys for a program.
 *  - one %s for program name 
 *  - the program needs to output a non-zero number on success and a zero on
 *    failure.
 *  - Add echo-es at start to always show keys for a program */
const static char program_detector[]= "(echo dwm; pstree -T -p $(xdotool getactivewindow getwindowpid)) | sed 's/[-+`|]\\+/\\n/g' | sed '/^\\s*$/d' | tr '[:upper:]' '[:lower:]' | sed 's/web content/firefox/g' | grep $(echo '%s' | tr '[:upper:]' '[:lower:]') | wc";
