









function __ziggy__func(){return "ziggy stardust"}

var __music_box={};

__music_box.ziggy = __ziggy__func;



if (typeof __music_box.ziggy !== "function") {
	$ERROR('#1: typeof __music_box.ziggy === "function". Actual: typeof __music_box.ziggy ==='+typeof __music_box.ziggy);
}





if (__music_box.ziggy() !== "ziggy stardust") {
	$ERROR('#2: __music_box.ziggy() === "ziggy stardust". Actual: __music_box.ziggy() ==='+__music_box.ziggy());
}



