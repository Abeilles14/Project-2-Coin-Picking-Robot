//
// Here are some MML tunes.
// Music Macro Language is used by basic programs.
// Check https://en.wikipedia.org/wiki/Music_Macro_Language for more information
// There is an on-line midi to MML converter that could be very handy:
// http://www.netmftoolbox.com/tools/midi_to_mml.php
// MIDI file pitch shifter: https://solmire.com/midieditor/
// MIDI file speed: https://www.ofoct.com/audio-converter/change-midi-speed-change-duration-of-midi-file.html

/*start tune*/
const char starttune[] = {
	"T220L8O2C<P9L4P5L8GP9L4P5L8EPL6AL480P580L6BL480P580L6AP580L6G#A#G#L15GPFPL1G"
};

/*coin tune*/
const char cointune[] = {
	"T220L8O2BL2O3E"
};

/*end tune*/
const char endtune[] = {
	"T220L410O1P410L18<G.L195>P195L17PL13CL195P195L12P12L13EL195P195L12P12L13GL195P195L12P12L13>CL195<P195L12P12L13>EL195<P195L12P12L4>GL4096<P4096L4P4>EL819<P819L17<G#L195>P195L16PL13CL195P195L12P12L13D#L195P195L12P12L13G#L195P195L12P12L13>CL195<P195L12P12L13>D#L195<P195L12P12L4>G#L4096<P4096L4P4>D#L819<P819L17<A#L195>P195L16PL13DL195P195L12P12L13FL195P195L12P12L13A#L195P195L12P12L13>DL195<P195L12P12L13>FL195<P195L12P12L4>A#L4096<P4096L4P4L12>A#L4096<P4096L12P12>A#L4096<P4096L12P12>A#L819<P819L18P.L2O3C"
};

/*main theme*/
const char maintheme[] = {
	"T150L1024O5P1024L17O2E.L15EL16O5P.L14O2EL16O5P.L15O2CL73O5P73L15O2EO5P.L13O2GL4O5P4L14O1GL4O5P4L13O2CL8O5P.L14O1GL8O5P.L15O1EL8O5P.L15O1AL16O5P.L14O1BL16O5P.L15O1A#L73O5P73L15O1AO5P.O1G.L73O5P73L15O2E.L73O5P73L16O2G.L73O5P73L14O2AL16O5P.L15O2FL74O5P74L14O2GL15O5P.O2EL16O5P.L14O2CL73O5P73L14O2DL73O5P73L15O1BL4O5P.L15O2GL73O5P73L14O2F#L73O5P73L14O2FL73O5P73L15O2D#L16O5P.L15O2EO5P.O1G#L73O5P73L15O1AL73O5P73L14O2CL16O5P.L15O1AL73O5P73L15O2CL73O5P73L15O2DL8O5P.L15O2GL73O5P73L14O2F#L73O5P73L14O2FL73O5P73L15O2D#L16O5P.L15O2EO5P.O3CL16O5P.L12O3CL14CL2O5P2L15O2GL73O5P73L14O2F#L73O5P73L14O2FL73O5P73L15O2D#L16O5P.L15O2EO5P.O1G#L73O5P73L15O1AL73O5P73L14O2CL16O5P.L15O1AL73O5P73L15O2CL73O5P73L15O2DL8O5P.L15O2D#L8O5P.L15O2DL8O5P.L13O2CL2O5P2L17O2C.L15CL16O5P.L14O2CL16O5P.L15O2CL73O5P73L15O2DO5P.L13O2EL73O5P73L15O2CL16O5P.L14O1AL73O5P73L14O1GL4O5P4L17O2C.L15CL16O5P.L14O2CL16O5P.L15O2CL73O5P73L15O2DL73O5P73L15O2EL2O5P.L17O2C.L15CL16O5P.L14O2CL16O5P.L15O2CL73O5P73L15O2DO5P.L13O2EL73O5P73L15O2CL16O5P.L14O1AL73O5P73L14O1GL4O5P4L17O2E.L15EL16O5P.L14O2EL16O5P.L15O2CL73O5P73L15O2EO5P.L13O2GL4O5P4L14O1GL4O5P4L13O2CL8O5P.L14O1GL8O5P.L15O1EL8O5P.L15O1AL16O5P.L14O1BL16O5P.L15O1A#L73O5P73L15O1AO5P.O1G.L73O5P73L15O2E.L73O5P73L16O2G.L73O5P73L14O2AL16O5P.L15O2FL74O5P74L14O2GL15O5P.O2EL16O5P.L14O2CL73O5P73L14O2DL73O5P73L15O1BL8O5P.L13O2EL73O5P73L15O2CL16O5P.L14O1GL8O5P.L15O1G#O5P.L13O1AL73O5P73L15O2FL16O5P.L14O2FL73O5P73L14O1AL4O5P4L15O1B.L73O5P73L9O2AAL16A.L73O5P73L16O2G.L73O5P73L16O2F.L69O5P69L13O2EL73O5P73L7O2CL73O5P73L14O1AL73O5P73L3O1GL71O5P71L13O2EL73O5P73L15O2CL16O5P.L14O1GL8O5P.L15O1G#O5P.L13O1AL73O5P73L15O2FL16O5P.L14O2FL73O5P73L14O1AL4O5P4L13O1AL73O5P73L15O2FL16O5P.L12O2FL16F.L73O5P73L16O2E.L73O5P73L16O2D.L69O5P69L13O2CL73O5P73L15O1EL16O5P.L14O1EL73O5P73L14O1CL4O5P4L17O2C.L15CL16O5P.L14O2CL16O5P.L15O2CL73O5P73L15O2DO5P.L13O2EL73O5P73L15O2CL16O5P.L14O1AL73O5P73L14O1GL4O5P4L17O2C.L15CL16O5P.L14O2CL16O5P.L15O2CL73O5P73L15O2DL73O5P73L15O2EL2O5P.L17O2C.L15CL16O5P.L14O2CL16O5P.L15O2CL73O5P73L15O2DO5P.L13O2EL73O5P73L15O2CL16O5P.L14O1AL73O5P73L14O1GL4O5P4L17O2E.L15EL16O5P.L14O2EL16O5P.L15O2CL73O5P73L15O2EO5P.L13O2GL4O5P4L14O1GL4O5P4L13O2EL76O5P76L15O2CL16O5P.L14O1GL8O5P.L15O1G#O5P.L13O1AL73O5P73L15O2FL16O5P.L14O2FL73O5P73L14O1AL4O5P4L15O1B.L73O5P73L9O2AAL16A.L73O5P73L16O2G.L73O5P73L16O2F.L69O5P69L13O2EL73O5P73L7O2CL73O5P73L14O1AL73O5P73L3O1GL69O5P69L13O2EL76O5P76L15O2CL16O5P.L14O1GL8O5P.L15O1G#O5P.L13O1AL73O5P73L15O2FL16O5P.L14O2FL73O5P73L14O1AL4O5P4L13O1AL73O5P73L15O2FL16O5P.L12O2FL16F.L73O5P73L16O2E.L73O5P73L16O2D.L69O5P69L13O2CL73O5P73L15O1EL16O5P.L14O1EL73O5P73L14O1C"
};

/*beyonce*/
const char beyonce[] = {
	"T150L2O5P.L19EL96P96L19BL96P96L19BL120P120L480>C#<P480L1920O2BL960O5P960O3D#L17O5P17L16P16L384O2BL17O5P17L16P16P16L480>C#L1920O3D#L17O5P17L19AL113P113L16P16P16L960>C#<P960L1920AP1920L960O2B>D#L17O5P17L19G#L120P120L960>C#EL1920<G#L960P960L1920O3D#L480O6EL17<P17L36G#.L52P52L960AP960P960P960AP960O3D#L1920O6C#L17<P17L27EL38P38L1920>EO2EO5P1920P1920P1920P1920>C#L960O2EO6EL17<P17L16P16L960O2BL640O5EL17P17L960O3D#L640O6C#L8<P8L640>C#L1920E<BP1920L960P960O3EL640O6EL17<P17L1920AO2FO5P1920L480BL17P17L1920>C#<P1920P1920L480O3D#L17O5P17L16P16L384O2BL17O5P17L1920>E<P1920L640>E<P640L384>C#L17<P17L27AL43P43L640O3D#L1920O5AP1920O2BO5P1920L960>C#<P960L1920O2FL17O5P17L58G#L35P.L960>C#<P960L1920>EL960O3D#L16O5P16L35G#L31P31L1920AAO2FO5P1920L960O2B>D#L1920O6C#L8<P8L1920>EE<P1920L640O2EL1920O6C#L480O3D#L17O5P17P17L384O2BL17O5P17L384>C#L16<P16L17BL213P213L1920EL960>C#L1920O3EO5P1920>EO3D#L17O5P17L640AL1920AP1920O2FL480O5EL17P17L480>C#<P480L1920O2BO5P1920L640O3D#L17O5P17L384O2BL17O5P17L1920O3D#O6C#L640<EP640L320P320L29>EL38<P38L27AL42P42L1920O2F>D#O5P1920O2BO5AP1920>C#<EL8P8L1920G#O3D#O6EC#L960EL8<P8L1920O3D#L960<FO5AP960P960P960L640P640L480EL16P16L27EL47P47L17P17L384>C#<P384P384L240P240L1920O2EO5P1920P1920L30O2E.L96O5P96L17P17L480O2BL1920O5EL17P17L960>C#L640O3D#L8O5P8L1920O3EO5P1920L960O3D#L1920O6C#<P1920>EL17<P17L480AL960BL1920AL384O2FL17O5P17L16P16P16L480>C#L1920O3D#L17O5P17P17L384O2BL17O5P17L1920>EO3D#L960O6C#L1920E<AL16P16L29AL37P37L16P16L1920O3D#O5P1920L640O2BL1920O5AP1920L16P16L19G#L96P96L16P16P16L1920>EC#L640EL17<P17L33G#P33L17P17L640O3D#L1920O5AP1920O2BL960O6C#<AL8P8P8L960O3D#O5P960L1920>EL960<F#L16P16P16L384O2BL17O5P17L16P16L480>C#L1920O3D#L8O5P8L1920>C#L640O3D#O5P640L1920EL960P960>EL1920EL17<P17P17L960EAL384O2FL17O5P17L16P16L1920>C#<P1920L960O2B>D#L17O5P17L384O2BL17O5P17L640O2EL1920O6E<P1920EO3D#O6EL960O2EL17O5P17L41O2EL28O5P28L960AP960P960AP960P960P960L640>C#L1920<P1920EL8P8L480>E<P480P480L1920F#L480F#>EL8<P8L960AL1920>C#O3D#O5P1920P1920L640AP640L1920EO2FL17O5P17L27EL40P40L960O3D#L1920O6EL960C#L1920<P1920L960>EL17<P17L640EP640L1920O2BL480O5F#L17P17L480O3D#L1920O6C#L17<P17L1920EL17EL1920O3EO6C#L640EL1920<P1920L320P320L384O3D#L17O5P17L16P16L1920O2FL480O5AL17P17L1920O3D#O5P1920P1920L640O2BL1920O6C#L17<P17L384O2BL17O5P17L1920>EC#L640O2EO5P640L1920>EL8<P8P8L1920AP1920L960>C#<P960O2BO5P960L480P480L1920O2FL8O5P8L1920>C#<F#L640>EL320<P320P320L384>EL8<P8L960O2FL1920>D#O5P1920P1920P1920L960>C#<G#L1920AL8P8L640>C#L960EL1920EL384O3D#L17O5P17L384O2BL17O5P17L1920>C#L480O3D#L240O5P240L15B.L44P44L960O3EO6C#<P960P960L1920>EO3D#O5P1920>EL17<P17L1920AL640O2FL1920O5EAL17P17L640EP640L960O2B>D#L1920O6C#L17<P17L384O2BL17O5P17L1920O2EO5P1920E>E<P1920>EL640O3D#L1920<EL8O5P8L1920O3D#O5EL960O2FO5P960P960L1920AP1920L960EL8P8L1920O3D#O5P1920L960>EL1920<F#>C#L960<P960>EL8<P8L1920G#L960AL1920O2FO6C#<P1920L960O2BO5P960P960L1920AL17P17L27EL42P42L1920>C#L960EO3D#O5P960L1920>EL17<P17L1920EP1920P1920L384O2BL17O5P17L640O3D#L960O6C#L8<P8L960EL1920>C#<P1920>E<P1920L960>EL1920O3EL17O5P17L1920AL384O2FL960O5P960L77AL33P.L384EP384P384L480>C#L1920O3D#L17O5P17L384O2BL17O5P17L960>EL1920O2EL384O6C#L320<P320L384O3D#L8O5P8P8P8L960O2FL640O5AP640P640P640L8P8L1920>E<P1920>C#L960<F#L1920F#>EL8<P8P8P8P8P8L960G#P960L1920>C#O3D#O5AL8P8P8L1920>C#L480EL640<P640L19>EL128<P128L384O2BL17O5P17L480O3D#L1920O6C#L8<P8P8L1920>EC#<P1920O3D#L960O6EL1920O3EL17O5P17L640AP640L1920O2FO5BL640AL17P17L480O2EO5P480L1920O2BO6C#L960O3D#L1920<EL17O5P17P17L384O2BL17O5P17P17L1920BL640>C#<P640L1920E>E<P1920L960P960P960L1920>EL480O2BL17O5P17L26AL46P46L17P17P17L960O3D#O5AL1920O2FO5P1920P1920P1920L960O3CL1920O6CC#<GL17P17L213G#L18P18L480O3D#O5P480L1920>EL960<P960L480>EL17<P17L33G#P33L1920O2F>D#L960O5AP960P960L1920O2BO5P1920L960P960L640>C#L17<P17L27EL41P41L16P16L960O3D#O5P960L1920>EL960EL1920<P1920L384>C#L17<P17P17P17L384EL17P17L384>C#L8<P8L640>E<P640L960>C#<P960L640O3EL960D#L17O5P17L960AL1920BL960AL384O2FL18O5P18L480O3CL1920O6C#<P1920P1920L960O3D#CL17O5P17L16P16L384O2BL17O5P17L16P16L1920>E<P1920O3D#O5P1920BP1920L960F#L1920>C#L17<P17L27AL43P43L1920O2FO5P1920O3D#L960<AO5P960P960L1920AO2AL960O5P960P960>C#L17<P17L128G#L19P19L960>C#L640EL480<P480L640O3D#L960O6EL17<P17L34G#L31P31L1920>C#<P1920L960O3D#O5P960P960AL1920P1920L8P8L640>E<P640L1920>EO3D#L480O6C#L17<P17L16P16L384O2BL17O5P17L960O3D#L640O6C#L17<P17BL960>EC#E<P960L1920EL960P960O3D#L17O5P17P17L640AL1920AP1920L384O2FL18O5P18L960O2AO5P960L1920O2BO6C#O2AL960>D#L17O5P17L384O2BL17O5P17L1920>E<EP1920O2BO5P1920L960BP960L1920>EO3D#L480O6C#L17<P17L29AL39P39L1920GO3CO6C<P1920>C#<P1920O2BFO5AO3CO5P1920AL8P8L640O3D#L1920O6C#<G#P1920L640>EL8<P8P8L960O2FO5P960L1920O2BO5AP1920EL960AL16P16L27EL47P47L960>EL1920C#<P1920P1920>EL640O3D#L17O5P17L384EP384O2BL17O5P17L960>C#L640O3D#L8O5P8L640>C#L960E<P960P960L320P320L1920>EL17<P17L1920AL960O2FL1920O5AP1920BL17P17L1920O2AL640>D#O5P640P640L960>C#<P960L1920O3CL17O5P17P17L384O2BL17O5P17L960O2BL1920O6E<P1920>C#<P1920>EO3D#<BO5BL17P17L27AL43P43L640AP640P640P640L1920AO2AO5P1920P1920P1920P1920L640>C#L16<P16L19G#L120P120L1920O3D#O6EL960EL1920<G#L384>C#L17<P17L34G#L31P31L960O2FL1920O5AP1920P1920L960AL1920P1920O3D#O6C#L8<P8L480>C#L1920<F#P1920L960P960L1920>EL480EL17<P17L1920EL960O2EO5P960L640O2BL1920EO4BL16>P16P16L384>C#L8<P8L480>EL1920O3EO6EC#L960O3D#"
};
