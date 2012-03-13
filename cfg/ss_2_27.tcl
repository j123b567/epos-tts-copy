#!wish
#############################################################################
# Visual Tcl v1.11 Project
#

#################################
# GLOBAL VARIABLES
#
global basedir; 
global ch_frequency; 
global ch_speed; 
global ch_volume; 
global file; 
global frequency; 
global lang2; 
global selectedButton; 
global speed; 
global vol; 
global widget; 
    set widget(dtext) {.top25.fra40.tex43}
    set widget(en_subor) {.top25.fra31.ent64}
    set widget(eokno) {.top30}
    set widget(etext) {.top25.fra40.tex23}
    set widget(helpokno) {.top17}
    set widget(hl_menu) {.top25.fra31}
    set widget(htext) {.top17.fra21.cpd28.03}
    set widget(ieokno) {.top27}
    set widget(ietext) {.top27.fra34.cpd35.03}
    set widget(mktext) {.top45.fra58.tex59}
    set widget(mokno) {.top25}
    set widget(pmokno) {.top45}
    set widget(pmtext) {.top45.fra53.cpd54.03}
    set widget(pokno) {.top34}
    set widget(rev,.top17) {helpokno}
    set widget(rev,.top17.fra21.cpd28.03) {htext}
    set widget(rev,.top17.fra21.tex26) {help}
    set widget(rev,.top25) {mokno}
    set widget(rev,.top25.fra31) {hl_menu}
    set widget(rev,.top25.fra31.ent64) {en_subor}
    set widget(rev,.top25.fra39.tex41) {text1}
    set widget(rev,.top25.fra40.tex23) {etext}
    set widget(rev,.top25.fra40.tex43) {dtext}
    set widget(rev,.top27) {ieokno}
    set widget(rev,.top27.fra34.cpd35.03) {ietext}
    set widget(rev,.top30) {eokno}
    set widget(rev,.top30.fra69.tex70) {etext}
    set widget(rev,.top30.tex32) {etext}
    set widget(rev,.top34) {pokno}
    set widget(rev,.top45) {pmokno}
    set widget(rev,.top45.fra53.cpd54.03) {pmtext}
    set widget(rev,.top45.fra58.tex59) {mktext}
    set widget(text1) {.top25.fra39.tex41}

#################################
# USER DEFINED PROCEDURES
#
proc init {argc argv} {

}

init $argc $argv


proc {helpshow} {hsubor} {
global widget
$widget(htext) delete 0.0 end
$widget(htext) insert 0.0 [exec more $hsubor]
}

proc {ieedit} {efile} {
global widget
Window show $widget(ieokno)
$widget(ietext) delete 0.0 end
$widget(ietext) insert 0.0 [exec more $efile]
}

proc {otvor} {okno} {
global tk_strictMotif
global file
    set tk_strictMotif 0
    set types {
	{"C Source Files"	{.c .cc .h}	}
	{"All files"		*}
	{"Text files"		{.txt .doc}	}
	{"Text files"		{}		TEXT}
	{"Tcl Scripts"		{.tcl}		TEXT}
	{"All Source Files"	{.tcl .c .h}	}
	{"Image Files"		{.gif}		}
	{"Image Files"		{.jpeg .jpg}	}
	{"Image Files"		""		{GIFF JPEG}}

    }
        set oldfile $file   
	set file [tk_getOpenFile -filetypes $types -parent $okno]
	if { $file != ""} {	
          $okno delete 0.0 end
          $okno insert 0.0 [exec more $file]
          } else { set file $oldfile}
}

proc {play} {} {
global basedir widget lang2 frequency speed ch_frequency ch_speed ch_volume
if {[$widget(text1) get 0.0 end]!=""} {
 set ch1 [open "$basedir/lng/$lang2/text.txt" w]
 puts $ch1 [$widget(text1) get 0.0 end]
 close $ch1
 $widget(dtext) delete 0.0 end
 $widget(etext) delete 0.0 end
 cd $basedir/src
 if [file isfile "vystup.wav"] {exec rm vystup.wav 2>NULL}
 $widget(dtext) insert 0.0  [exec ./ss --ch_fr=$ch_frequency --ch_vo=$ch_volume --ch_sp=$ch_speed --f_neutral=$frequency --t_neutral=$speed --language=$lang2 2>err]
 $widget(etext) insert 0.0 [exec more err]
 catch {exec vplay /tmp/vystup.wav 2>NULL &}
}
}

proc {uloz} {okno sfile} {
global widget
set ch2 [open $sfile w]
puts  $ch2  [$widget($okno) get 0.0 end]
close $ch2
}

proc {main} {argc argv} {
global basedir lang2 widget ch_frequency ch_speed ch_volume frequency vol speed
set basedir /usr/lib/ss
set lang2 slovak
set wplayer vplay
set ch_frequency yes
set ch_speed yes
set ch_volume yes
set speed 100
set vol 80
set frequency 80
Window  show $widget(eokno)
Window  hide $widget(eokno)
Window  show $widget(ieokno)
Window  hide $widget(ieokno)
}

proc {Window} {args} {
global vTcl
    set cmd [lindex $args 0]
    set name [lindex $args 1]
    set newname [lindex $args 2]
    set rest [lrange $args 3 end]
    if {$name == "" || $cmd == ""} {return}
    if {$newname == ""} {
        set newname $name
    }
    set exists [winfo exists $newname]
    switch $cmd {
        show {
            if {$exists == "1" && $name != "."} {wm deiconify $name; return}
            if {[info procs vTclWindow(pre)$name] != ""} {
                eval "vTclWindow(pre)$name $newname $rest"
            }
            if {[info procs vTclWindow$name] != ""} {
                eval "vTclWindow$name $newname $rest"
            }
            if {[info procs vTclWindow(post)$name] != ""} {
                eval "vTclWindow(post)$name $newname $rest"
            }
        }
        hide    { if $exists {wm withdraw $newname; return} }
        iconify { if $exists {wm iconify $newname; return} }
        destroy { if $exists {destroy $newname; return} }
    }
}

#################################
# VTCL GENERATED GUI PROCEDURES
#

proc vTclWindow. {base} {
    if {$base == ""} {
        set base .
    }
    ###################
    # CREATING WIDGETS
    ###################
    wm focusmodel $base passive
    wm geometry $base 1x1+0+0
    wm maxsize $base 785 570
    wm minsize $base 1 1
    wm overrideredirect $base 0
    wm resizable $base 1 1
    wm withdraw $base
    wm title $base "vt.tcl"
    bind $base <Key-Return> {
        $widget(pmtext) delete 0.0 end
$widget(pmtext) insert [exec more $pmfile]
    }
    ###################
    # SETTING GEOMETRY
    ###################
}

proc vTclWindow.top17 {base} {
    if {$base == ""} {
        set base .top17
    }
    if {[winfo exists $base]} {
        wm deiconify $base; return
    }
    ###################
    # CREATING WIDGETS
    ###################
    toplevel $base -class Toplevel
    wm focusmodel $base passive
    wm geometry $base 866x566+92+159
    wm maxsize $base 1009 738
    wm minsize $base 1 1
    wm overrideredirect $base 0
    wm resizable $base 1 1
    wm title $base "Help"
    frame $base.fra18  -borderwidth 2 -height 35 -relief groove -width 338 
    menubutton $base.fra18.men24  -menu .top17.fra18.men24.m -padx 4 -pady 3 -relief raised -text Index 
    menu $base.fra18.men24.m  -cursor {} 
    $base.fra18.men24.m add command  -command {global basedir
helpshow $basedir/doc/COPYING}  -label COPYING 
    $base.fra18.men24.m add command  -command {global basedir
helpshow $basedir/doc/Changes}  -label Changes 
    $base.fra18.men24.m add command  -command {global basedir
helpshow $basedir/doc/Files} -label Files 
    $base.fra18.men24.m add command  -command {global basedir
helpshow $basedir/doc/Intro} -label Intro 
    $base.fra18.men24.m add command  -command {global basedir
helpshow $basedir/doc/Todo} -label Todo 
    button $base.fra18.but25  -padx 9 -pady 3 -text Search 
    menubutton $base.fra18.men27  -menu .top17.fra18.men27.m -padx 4 -pady 3 -relief raised  -text Language 
    menu $base.fra18.men27.m  -cursor {} 
    $base.fra18.men27.m add command   -command {global basedir
set helpdir $basedir/doc/czech
set lang Czech}  -label Czech 
    $base.fra18.men27.m add command   -command {global basedir
set helpdir  $basedir/doc/english
set lang English}  -label English 
    label $base.fra18.lab29  -borderwidth 0 -foreground #0000e6 -padx 20 -relief raised  -text English -textvariable lang 
    frame $base.fra21  -borderwidth 2 -height 312 -relief groove -width 125 
    frame $base.fra21.cpd28  -borderwidth 1 -height 30 -relief raised -width 30 
    scrollbar $base.fra21.cpd28.01  -borderwidth 1 -command {.top17.fra21.cpd28.03 xview} -orient horiz  -width 10 
    scrollbar $base.fra21.cpd28.02  -borderwidth 1 -command {.top17.fra21.cpd28.03 yview} -orient vert  -width 10 
    text $base.fra21.cpd28.03  -background #fcf8f6 -font {vga 14 {}} -foreground #000064 -height 1  -width 8 -xscrollcommand {.top17.fra21.cpd28.01 set}  -yscrollcommand {.top17.fra21.cpd28.02 set} 
    button $base.but22  -command {Window hide $widget(helpokno)} -padx 9 -pady 3 -text Close 
    ###################
    # SETTING GEOMETRY
    ###################
    pack $base.fra18  -in .top17 -anchor center -expand 0 -fill x -side top 
    pack $base.fra18.men24  -in .top17.fra18 -anchor w -expand 0 -fill none -side left 
    pack $base.fra18.but25  -in .top17.fra18 -anchor center -expand 0 -fill none -side left 
    pack $base.fra18.men27  -in .top17.fra18 -anchor center -expand 0 -fill none -side left 
    pack $base.fra18.lab29  -in .top17.fra18 -anchor center -expand 0 -fill none -side right 
    pack $base.fra21  -in .top17 -anchor center -expand 1 -fill both -side top 
    pack $base.fra21.cpd28  -in .top17.fra21 -anchor center -expand 1 -fill both -side top 
    grid columnconf $base.fra21.cpd28 0 -weight 1
    grid rowconf $base.fra21.cpd28 0 -weight 1
    grid $base.fra21.cpd28.01  -in .top17.fra21.cpd28 -column 0 -row 1 -columnspan 1 -rowspan 1  -sticky ew 
    grid $base.fra21.cpd28.02  -in .top17.fra21.cpd28 -column 1 -row 0 -columnspan 1 -rowspan 1  -sticky ns 
    grid $base.fra21.cpd28.03  -in .top17.fra21.cpd28 -column 0 -row 0 -columnspan 1 -rowspan 1  -sticky nesw 
    pack $base.but22  -in .top17 -anchor center -expand 0 -fill x -side top
}

proc vTclWindow.top25 {base} {
    if {$base == ""} {
        set base .top25
    }
    if {[winfo exists $base]} {
        wm deiconify $base; return
    }
    ###################
    # CREATING WIDGETS
    ###################
    toplevel $base -class Toplevel \
        -cursor xterm 
    wm focusmodel $base passive
    wm geometry $base 832x505+95+181
    wm maxsize $base 1009 738
    wm minsize $base 1 1
    wm overrideredirect $base 0
    wm resizable $base 1 1
    wm deiconify $base
    wm title $base "SS  Speech synthesis"
    frame $base.fra31 \
        -borderwidth 2 -height 48 -relief groove -width 207 
    menubutton $base.fra31.men32 \
        -menu .top25.fra31.men32.m -padx 4 -pady 3 -relief raised -text File \
        -underline 0 -width 7 
    menu $base.fra31.men32.m \
        -cursor {} 
    $base.fra31.men32.m add command \
        -command {otvor $widget(text1)} -label Open 
    $base.fra31.men32.m add command \
        -label Save 
    $base.fra31.men32.m add command \
        -label {Save as} 
    $base.fra31.men32.m add command \
        -command {Window hide  $widget(mokno)
exit} -label Quit 
    menubutton $base.fra31.men33 \
        -menu .top25.fra31.men33.m -padx 4 -pady 3 -relief raised \
        -text Options -underline 0 -width 7 
    menu $base.fra31.men33.m \
        -cursor {} 
    $base.fra31.men33.m add cascade \
        -label Output -menu .top25.fra31.men33.m.men37 
    $base.fra31.men33.m add command \
        -command {Window show $widget(pokno)} -label Preferencies 
    menu $base.fra31.men33.m.men37
    $base.fra31.men33.m.men37 add radiobutton \
        -label {TD Psola} -value {TD Psola} 
    $base.fra31.men33.m.men37 add radiobutton \
        -label LPC -value LPC 
    $base.fra31.men33.m.men37 add radiobutton \
        -label MBROLA -value MBROLA 
    $base.fra31.men33.m.men37 add separator
    $base.fra31.men33.m.men37 add radiobutton \
        -label Text -value Text 
    $base.fra31.men33.m.men37 add radiobutton \
        -label {Wav file} -value {Wav file} 
    $base.fra31.men33.m.men37 add radiobutton \
        -label /dev/dsp -value /dev/dsp 
    menubutton $base.fra31.men34 \
        -menu .top25.fra31.men34.m -padx 4 -pady 3 -relief raised -text Tools \
        -underline 0 -width 7 -wraplength 200 
    menu $base.fra31.men34.m \
        -cursor {} 
    $base.fra31.men34.m add command \
        -command {Window show $widget(eokno)} -label {Show errors} 
    $base.fra31.men34.m add command \
        -command {Window show $widget(pmokno)} -label {Project Maker} 
    $base.fra31.men34.m add command \
        -label {Diphone edit} 
    menubutton $base.fra31.men35 \
        -height 1 -menu .top25.fra31.men35.m -padx 4 -pady 1 -relief raised \
        -text Help -underline 0 -width 15 
    menu $base.fra31.men35.m \
        -cursor {} 
    $base.fra31.men35.m add command \
        -label About 
    $base.fra31.men35.m add command \
        -command {Window show $widget(helpokno)} -label Help 
    button $base.fra31.cpd60 \
        -background #3836df -command play -font {vga 14 {}} \
        -foreground #f8ee00 -padx 20 -pady 7 -text {Play
} 
    label $base.fra31.cpd46 \
        -borderwidth 1 -height 1 -relief groove -text frequency -width 10 
    label $base.fra31.cpd47 \
        -borderwidth 1 -height 1 -relief groove -text speed -width 10 
    label $base.fra31.cpd48 \
        -borderwidth 1 -relief raised -text volume -width 10 
    scale $base.fra31.cpd53 \
        -borderwidth 1 -from 40.0 -orient horiz -showvalue 0 -to 200.0 \
        -variable frequency -width 10 
    scale $base.fra31.cpd54 \
        -borderwidth 1 -from 40.0 -orient horiz -showvalue 0 -to 200.0 \
        -variable speed -width 10 
    scale $base.fra31.cpd55 \
        -activebackground #d9d9d9 -borderwidth 1 -command {exec mixer pcm} \
        -orient horiz -showvalue 0 -variable vol -width 10 
    label $base.fra31.cpd57 \
        -borderwidth 1 -relief raised -text 80 -textvariable frequency 
    label $base.fra31.cpd58 \
        -borderwidth 1 -relief raised -text 100 -textvariable speed 
    label $base.fra31.cpd59 \
        -borderwidth 1 -relief raised -text 80 -textvariable vol 
    checkbutton $base.fra31.cpd61 \
        -anchor w -offvalue no -onvalue yes -selectcolor #1a16cc \
        -variable ch_frequency 
    checkbutton $base.fra31.cpd62 \
        -anchor w -offvalue no -onvalue yes -variable ch_speed 
    checkbutton $base.fra31.cpd63 \
        -anchor w -offvalue no -onvalue yes -variable ch_volume 
    entry $base.fra31.ent64 \
        -textvariable file 
    bind $base.fra31.ent64 <Key-Return> {
        $widget(text1) delete 0.0 end
if [file isfile $file] {
 $widget(text1) insert 0.0 [exec more $file]
}
    }
    menubutton $base.fra31.men19 \
        -font {lucidabright 18 italic} -foreground #0000d6 \
        -menu .top25.fra31.men19.m -padx 4 -pady 3 -text slovak \
        -textvariable lang2 
    menu $base.fra31.men19.m \
        -cursor {} 
    $base.fra31.men19.m add command \
        -command {set lang2 czech} -label Czech 
    $base.fra31.men19.m add command \
        -command {set lang2 slovak} -label Slovak 
    $base.fra31.men19.m add command \
        -command {set lang2 german} -label German 
    $base.fra31.men19.m add command \
        -command {set lang2 french} -label French 
    frame $base.fra39 \
        -borderwidth 2 -height 209 -relief groove -width 733 
    text $base.fra39.tex41 \
        -font {vga 14 {}} -height 7 -width 1 \
        -yscrollcommand {.top25.fra39.scr42 set} 
    scrollbar $base.fra39.scr42 \
        -command {.top25.fra39.tex41 yview} -orient vert 
    frame $base.fra40 \
        -borderwidth 2 -height 131 -relief groove -width 249 
    scrollbar $base.fra40.scr44 \
        -command {.top25.fra40.tex43 yview} -orient vert -width 10 
    text $base.fra40.tex43 \
        -font {vga 14 {}} -foreground #0000e2 -height 4 -width 68 \
        -yscrollcommand {.top25.fra40.scr44 set} 
    scrollbar $base.fra40.scr24 \
        -command {$widget(etext) yview set} -orient vert 
    text $base.fra40.tex23 \
        -background #efefef -foreground #f60000 -height 5 -width 55 \
        -yscrollcommand {.top25.fra40.scr24 set} 
    ###################
    # SETTING GEOMETRY
    ###################
    pack $base.fra31 \
        -in .top25 -anchor n -expand 0 -fill both -side top 
    pack $base.fra31.men32 \
        -in .top25.fra31 -anchor n -expand 0 -fill none -side left 
    pack $base.fra31.men33 \
        -in .top25.fra31 -anchor n -expand 0 -fill none -side left 
    pack $base.fra31.men34 \
        -in .top25.fra31 -anchor n -expand 0 -fill none -side left 
    pack $base.fra31.men35 \
        -in .top25.fra31 -anchor n -expand 0 -fill none -side right 
    pack $base.fra31.cpd60 \
        -in .top25.fra31 -anchor center -expand 0 -fill none -padx 20 \
        -side left 
    pack $base.fra31.cpd46 \
        -in .top25.fra31 -anchor w -expand 0 -fill none -side top 
    pack $base.fra31.cpd47 \
        -in .top25.fra31 -anchor w -expand 0 -fill none -side top 
    pack $base.fra31.cpd48 \
        -in .top25.fra31 -anchor w -expand 0 -fill none -side left 
    place $base.fra31.cpd53 \
        -x 435 -y 4 -width 103 -height 16 -anchor nw -bordermode ignore 
    place $base.fra31.cpd54 \
        -x 435 -y 21 -width 104 -height 16 -anchor nw -bordermode ignore 
    place $base.fra31.cpd55 \
        -x 436 -y 38 -width 104 -height 16 -anchor nw -bordermode ignore 
    place $base.fra31.cpd57 \
        -x 543 -y 5 -width 39 -height 12 -anchor nw -bordermode ignore 
    place $base.fra31.cpd58 \
        -x 543 -y 22 -width 39 -height 12 -anchor nw -bordermode ignore 
    place $base.fra31.cpd59 \
        -x 543 -y 38 -width 39 -height 12 -anchor nw -bordermode ignore 
    place $base.fra31.cpd61 \
        -x 585 -y 5 -width 21 -height 15 -anchor nw -bordermode ignore 
    place $base.fra31.cpd62 \
        -x 585 -y 20 -width 25 -height 16 -anchor nw -bordermode ignore 
    place $base.fra31.cpd63 \
        -x 585 -y 36 -width 21 -height 14 -anchor nw -bordermode ignore 
    place $base.fra31.ent64 \
        -x 5 -y 30 -width 183 -height 22 -anchor nw -bordermode ignore 
    place $base.fra31.men19 \
        -x 620 -y 25 -width 109 -height 24 -anchor nw -bordermode ignore 
    pack $base.fra39 \
        -in .top25 -anchor n -expand 1 -fill both -side top 
    pack $base.fra39.tex41 \
        -in .top25.fra39 -anchor center -expand 1 -fill both -side left 
    pack $base.fra39.scr42 \
        -in .top25.fra39 -anchor center -expand 1 -fill y -side top 
    pack $base.fra40 \
        -in .top25 -anchor n -expand 1 -fill both -side top 
    pack $base.fra40.scr44 \
        -in .top25.fra40 -anchor n -expand 0 -fill y -ipadx 1 -side right 
    pack $base.fra40.tex43 \
        -in .top25.fra40 -anchor w -expand 1 -fill both -side top 
    pack $base.fra40.scr24 \
        -in .top25.fra40 -anchor center -expand 0 -fill y -side right 
    pack $base.fra40.tex23 \
        -in .top25.fra40 -anchor center -expand 1 -fill both -side bottom 
}

proc vTclWindow.top34 {base} {
    if {$base == ""} {
        set base .top34
    }
    if {[winfo exists $base]} {
        wm deiconify $base; return
    }
    ###################
    # CREATING WIDGETS
    ###################
    toplevel $base -class Toplevel
    wm focusmodel $base passive
    wm geometry $base 582x344+99+242
    wm maxsize $base 1009 738
    wm minsize $base 1 1
    wm overrideredirect $base 0
    wm resizable $base 1 1
    wm title $base "Preferencies"
    frame $base.fra44  -borderwidth 2 -height 75 -relief groove -width 262 
    label $base.fra44.lab45  -anchor e -borderwidth 1 -relief raised -text {Base Directory : }  -width 15 
    entry $base.fra44.ent46  -textvariable basedir 
    button $base.fra44.but47  -padx 9 -pady 3 -text Browse 
    frame $base.fra48  -borderwidth 2 -height 46 -relief groove -width 125 
    label $base.fra48.lab49  -anchor e -borderwidth 1 -relief raised -text {Wav player : }  -width 15 
    entry $base.fra48.ent50  -textvariable wplayer 
    button $base.fra48.but51  -padx 9 -pady 3 -text Browse 
    frame $base.fra52  -borderwidth 2 -height 75 -relief groove -width 125 
    label $base.fra52.lab54  -anchor e -borderwidth 1 -relief raised -text {Language :} -width 15 
    radiobutton $base.fra52.rad55  -text Czech -value czech -variable lang2 
    radiobutton $base.fra52.rad56  -text {Slovak
} -value slovak -variable lang2 
    radiobutton $base.fra52.rad57  -text German -value german -variable lang2 
    radiobutton $base.fra52.rad18  -text French -value french -variable lang2 
    frame $base.fra59  -borderwidth 2 -height 75 -relief groove -width 125 
    label $base.fra59.lab60  -anchor e -borderwidth 1 -justify left -relief raised  -text {Synth metod:} -width 15 
    radiobutton $base.fra59.rad61  -text {none - text output} -value none -variable synth 
    radiobutton $base.fra59.rad62  -text {TD PSOLA} -value tdpsola -variable synth 
    radiobutton $base.fra59.rad63  -text LPC -value lpc -variable synth 
    frame $base.fra64  -borderwidth 2 -height 75 -relief groove -width 125 
    label $base.fra64.lab65  -anchor e -borderwidth 1 -relief raised -text {Edit INI Files :}  -width 15 
    entry $base.fra64.ent66  -textvariable iefile 
    bind $base.fra64.ent66 <Key-Return> {
        ieedit $iefile
    }
    button $base.fra64.but67   -command {otvor $widget(ietext)
set iefile $file
Window show $widget(ieokno)}  -padx 9 -pady 3 -text Browse 
    button $base.but68  -command {Window hide $widget(pokno)} -padx 9 -pady 3 -text Close 
    ###################
    # SETTING GEOMETRY
    ###################
    grid columnconf $base 0 -weight 1
    grid rowconf $base 0 -weight 1
    grid rowconf $base 1 -weight 1
    pack $base.fra44  -in .top34 -anchor center -expand 0 -fill x -side top 
    pack $base.fra44.lab45  -in .top34.fra44 -anchor center -expand 0 -fill none -side left 
    pack $base.fra44.ent46  -in .top34.fra44 -anchor center -expand 1 -fill x -side left 
    pack $base.fra44.but47  -in .top34.fra44 -anchor center -expand 0 -fill none -side right 
    pack $base.fra48  -in .top34 -anchor center -expand 0 -fill x -side top 
    pack $base.fra48.lab49  -in .top34.fra48 -anchor center -expand 0 -fill none -side left 
    pack $base.fra48.ent50  -in .top34.fra48 -anchor center -expand 1 -fill x -side left 
    pack $base.fra48.but51  -in .top34.fra48 -anchor center -expand 0 -fill none -side right 
    pack $base.fra52  -in .top34 -anchor center -expand 0 -fill x -side top 
    pack $base.fra52.lab54  -in .top34.fra52 -anchor n -expand 0 -fill none -side left 
    pack $base.fra52.rad55  -in .top34.fra52 -anchor nw -expand 0 -fill none -padx 20 -side left 
    pack $base.fra52.rad56  -in .top34.fra52 -anchor w -expand 0 -fill none -padx 20 -side left 
    pack $base.fra52.rad57  -in .top34.fra52 -anchor nw -expand 0 -fill none -padx 20 -side left 
    pack $base.fra52.rad18  -in .top34.fra52 -anchor nw -expand 0 -fill none -side left 
    pack $base.fra59  -in .top34 -anchor center -expand 0 -fill x -side top 
    pack $base.fra59.lab60  -in .top34.fra59 -anchor nw -expand 0 -fill none -side left 
    pack $base.fra59.rad61  -in .top34.fra59 -anchor nw -expand 0 -fill none -padx 20 -side top 
    pack $base.fra59.rad62  -in .top34.fra59 -anchor nw -expand 0 -fill none -padx 20 -side top 
    pack $base.fra59.rad63  -in .top34.fra59 -anchor nw -expand 0 -fill none -padx 20 -side top 
    pack $base.fra64  -in .top34 -anchor center -expand 0 -fill x -side top 
    pack $base.fra64.lab65  -in .top34.fra64 -anchor center -expand 0 -fill none -side left 
    pack $base.fra64.ent66  -in .top34.fra64 -anchor center -expand 1 -fill x -side left 
    pack $base.fra64.but67  -in .top34.fra64 -anchor center -expand 0 -fill none -side right 
    pack $base.but68  -in .top34 -anchor center -expand 0 -fill x -side bottom
}

proc vTclWindow.top45 {base} {
    if {$base == ""} {
        set base .top45
    }
    if {[winfo exists $base]} {
        wm deiconify $base; return
    }
    ###################
    # CREATING WIDGETS
    ###################
    toplevel $base -class Toplevel
    wm focusmodel $base passive
    wm geometry $base 832x582+114+112
    wm maxsize $base 1009 738
    wm minsize $base 1 1
    wm overrideredirect $base 0
    wm resizable $base 1 1
    wm title $base "Projekt maker"
    bind $base <Key-F2> {
        set oznam Saving
uloz pmtext $pmfile
set oznam Editing
    }
    bind $base <Key-F3> {
        otvor $widget(pmtext)
set pmfile $file
    }
    bind $base <Key-F9> {
        $widget(mktext) delete 0.0 end
cd $basedir/src
exec xterm -e make 2>chyba
$widget(mktext) insert 0.0 [exec more chyba]
    }
    frame $base.fra46  -borderwidth 2 -height 43 -relief groove -width 125 
    menubutton $base.fra46.men57  -menu .top45.fra46.men57.m -padx 9 -pady 4 -relief raised -text File  -width 5 
    menu $base.fra46.men57.m  -cursor {} 
    $base.fra46.men57.m add command  -command {otvor $widget(pmtext)
set pmfile $file} -label Open 
    $base.fra46.men57.m add command  -command {cp $pmfile /temp/zaloha
uloz pmtext $pmfile} -label Save 
    $base.fra46.men57.m add command  -command {set pmfile [tk_getSaveFile]
uloz pmtext $pmfile}  -label {Save As} 
    $base.fra46.men57.m add separator
    $base.fra46.men57.m add command  -label Close 
    button $base.fra46.but52   -command {$widget(pmtext) delete 0.0 end
$widget(pmtext) insert 0.0 [exec more /temp/zaloha]}  -padx 9 -pady 3 -text Undo -width 5 
    entry $base.fra46.ent55  -textvariable pmfile 
    bind $base.fra46.ent55 <Key-Return> {
        $widget(pmtext) delete 0.0 end
set oznam Loading
if [file isfile $pmfile] {
 $widget(pmtext) insert 0.0 [exec more $pmfile]
set oznam Editing 
}
    }
    label $base.fra46.lab35  -borderwidth 1 -relief raised -text Saving -textvariable oznam  -width 20 
    frame $base.fra53  -borderwidth 2 -height 203 -relief groove -width 342 
    frame $base.fra53.cpd54  -borderwidth 1 -height 83 -relief raised -width 30 
    scrollbar $base.fra53.cpd54.01  -borderwidth 1 -command {.top45.fra53.cpd54.03 xview} -orient horiz  -width 10 
    scrollbar $base.fra53.cpd54.02  -borderwidth 1 -command {.top45.fra53.cpd54.03 yview} -orient vert  -width 10 
    text $base.fra53.cpd54.03  -background #0818be -font {vga 14 {}} -foreground #fce800 -height 1  -insertbackground #fafcf6 -width 8  -xscrollcommand {.top45.fra53.cpd54.01 set}  -yscrollcommand {.top45.fra53.cpd54.02 set} 
    bind $base.fra53.cpd54.03 <Key-F2> {
        set oznam Saving
uloz pmtext $pmfile
    }
    frame $base.fra18  -borderwidth 2 -height 75 -relief groove -width 125 
    frame $base.fra18.cpd41  -borderwidth 2 -height 75 -relief raised -width 125 
    label $base.fra18.cpd41.01  -borderwidth 1 -foreground #da0000 -text { F1 } 
    button $base.fra18.cpd41.02  -padx 9 -pady 3 -relief flat -text Help 
    frame $base.fra18.fra38  -borderwidth 2 -height 75 -relief raised -width 125 
    label $base.fra18.fra38.lab39  -borderwidth 1 -foreground #da0000 -text { F2 } 
    button $base.fra18.fra38.but40  -padx 9 -pady 3 -relief flat -text Save 
    frame $base.fra18.cpd43  -borderwidth 2 -height 75 -relief raised -width 125 
    label $base.fra18.cpd43.01  -borderwidth 1 -foreground #da0000 -text { F3 } 
    button $base.fra18.cpd43.02  -padx 9 -pady 3 -relief flat -text Open 
    frame $base.fra18.cpd44  -borderwidth 2 -height 75 -relief raised -width 125 
    label $base.fra18.cpd44.01  -borderwidth 1 -foreground #da0000 -text { F7 } 
    button $base.fra18.cpd44.02  -padx 9 -pady 3 -relief flat -text Search 
    frame $base.fra18.cpd45  -borderwidth 2 -height 75 -relief raised -width 125 
    label $base.fra18.cpd45.01  -borderwidth 1 -foreground #da0000 -text { F9 } 
    button $base.fra18.cpd45.02  -padx 9 -pady 3 -relief flat -text Make 
    frame $base.fra18.cpd46  -borderwidth 2 -height 75 -relief raised -width 125 
    label $base.fra18.cpd46.01  -borderwidth 1 -foreground #da0000 -text { F10 } 
    button $base.fra18.cpd46.02  -padx 9 -pady 3 -relief flat -text Close 
    frame $base.fra58  -borderwidth 2 -height 75 -relief groove -width 125 
    text $base.fra58.tex59  -height 4 
    ###################
    # SETTING GEOMETRY
    ###################
    pack $base.fra46  -in .top45 -anchor center -expand 0 -fill x -side top 
    pack $base.fra46.men57  -in .top45.fra46 -anchor center -expand 0 -fill none -side left 
    pack $base.fra46.but52  -in .top45.fra46 -anchor center -expand 0 -fill none -side left 
    pack $base.fra46.ent55  -in .top45.fra46 -anchor center -expand 1 -fill x -pady 3 -side left 
    pack $base.fra46.lab35  -in .top45.fra46 -anchor center -expand 0 -fill none -side right 
    pack $base.fra53  -in .top45 -anchor center -expand 1 -fill both -side top 
    pack $base.fra53.cpd54  -in .top45.fra53 -anchor center -expand 1 -fill both -side top 
    grid columnconf $base.fra53.cpd54 0 -weight 1
    grid rowconf $base.fra53.cpd54 0 -weight 1
    grid $base.fra53.cpd54.01  -in .top45.fra53.cpd54 -column 0 -row 1 -columnspan 1 -rowspan 1  -sticky ew 
    grid $base.fra53.cpd54.02  -in .top45.fra53.cpd54 -column 1 -row 0 -columnspan 1 -rowspan 1  -sticky ns 
    grid $base.fra53.cpd54.03  -in .top45.fra53.cpd54 -column 0 -row 0 -columnspan 1 -rowspan 1  -sticky nesw 
    pack $base.fra18  -in .top45 -anchor center -expand 0 -fill x -side top 
    pack $base.fra18.cpd41  -in .top45.fra18 -anchor center -expand 0 -fill none -side left 
    pack $base.fra18.cpd41.01  -in .top45.fra18.cpd41 -anchor center -expand 1 -fill both -side left 
    pack $base.fra18.cpd41.02  -in .top45.fra18.cpd41 -anchor center -expand 0 -fill none -side left 
    pack $base.fra18.fra38  -in .top45.fra18 -anchor center -expand 0 -fill none -side left 
    pack $base.fra18.fra38.lab39  -in .top45.fra18.fra38 -anchor center -expand 1 -fill both -side left 
    pack $base.fra18.fra38.but40  -in .top45.fra18.fra38 -anchor center -expand 0 -fill none  -side right 
    pack $base.fra18.cpd43  -in .top45.fra18 -anchor center -expand 0 -fill none -side left 
    pack $base.fra18.cpd43.01  -in .top45.fra18.cpd43 -anchor center -expand 1 -fill both -side left 
    pack $base.fra18.cpd43.02  -in .top45.fra18.cpd43 -anchor center -expand 0 -fill none  -side right 
    pack $base.fra18.cpd44  -in .top45.fra18 -anchor center -expand 0 -fill none -side left 
    pack $base.fra18.cpd44.01  -in .top45.fra18.cpd44 -anchor center -expand 1 -fill both -side left 
    pack $base.fra18.cpd44.02  -in .top45.fra18.cpd44 -anchor center -expand 0 -fill none  -side right 
    pack $base.fra18.cpd45  -in .top45.fra18 -anchor center -expand 0 -fill none -side left 
    pack $base.fra18.cpd45.01  -in .top45.fra18.cpd45 -anchor center -expand 1 -fill both -side left 
    pack $base.fra18.cpd45.02  -in .top45.fra18.cpd45 -anchor center -expand 0 -fill none  -side right 
    pack $base.fra18.cpd46  -in .top45.fra18 -anchor center -expand 0 -fill none -side left 
    pack $base.fra18.cpd46.01  -in .top45.fra18.cpd46 -anchor center -expand 1 -fill both -side left 
    pack $base.fra18.cpd46.02  -in .top45.fra18.cpd46 -anchor center -expand 0 -fill none  -side right 
    pack $base.fra58  -in .top45 -anchor center -expand 0 -fill x -side top 
    pack $base.fra58.tex59  -in .top45.fra58 -anchor center -expand 1 -fill x -side top
}

Window show .
Window show .top25

main $argc $argv
