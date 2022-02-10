
/* SETTINGS FOR SHOWKEYS */

const static int initial_width = 800;
const static int initial_height = 800;

const static char key_dir[]      = "/usr/share/showkeys"; /* directory containing the key defs (without trailing /) */

const static char color_boder[]     = "#88c0d0"; /* window border */
const static char color_bg[]        = "#3b4252"; /* background    */
const static char color_search_bg[] = "#4c566a"; /* background    */
const static char color_fg[]        = "#eceff4"; /* foreground    */
const static char color_title[]     = "#ffffff"; /* foreground    */
const static char color_dim_fg[]    = "#929aaa"; /* modifiers     */

const static unsigned int color_count = 6;
const static char* title_colors[] = {
	"#bf616a",
	"#d08770",
	"#ebcb8b",
	"#a3be8c",
	"#5e81ac",
	"#b48ead",
};

const static char font[]           = "Ubuntu";
const static int  font_size        = 12;
const static int  title_font_size  = 20;
const static int  search_font_size = 16;

const static int  border_padding   = 10;
const static int  title_padding    = 3;
const static int  padding_below_title = 5;
const static int  key_padding      = 2;
const static int  column_padding   = 5;
const static int  search_padding   = 7;

/* list of running programs */
/* add echo-es at the start to make a program always visible in the list */

const static char program_list[] = "echo dwm; pstree -T -p $(xdotool getactivewindow getwindowpid) | sed 's/[-+`|]\\+/\\n/g' | sed '/^\\s*$/d' | sed 's/([0-9]*)$//' | tr '[:upper:]' '[:lower:]' | sed 's/web content/firefox/g' | sed 's/^tmux.*$/tmux\\n'$(tmux ls -F '#W #{window_activity}' | sort -k 2 | tail -n 1 | awk '{print $1}')'/'";
