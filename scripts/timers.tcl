# shroudBNC - an object-oriented framework for IRC
# Copyright (C) 2005 Gunnar Beutner
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

package require bnc 0.2

internalbind pulse sbnc:tcltimers

# entry point to sbnc's timer system
proc sbnc:tcltimers {time} {
	foreach user [bncuserlist] {
		setctx $user

		namespace eval [getns] {
			if {![info exists utimers]} { set utimers "" }
			if {![info exists timers]} { set timers "" }
		}

		upvar [getns]::utimers utimers
		upvar [getns]::timers timers

		foreach timer $utimers {
			if {$time > [lindex $timer 0]} {
				catch {eval [lindex $timer 1]}
				setctx $user
				killutimer [lindex $timer 2]
			}
		}

		foreach timer $timers {
			if {$time > [lindex $timer 0]} {
				catch {eval [lindex $timer 1]}
				setctx $user
				killtimer [lindex $timer 2]
			}
		}
	}
}

proc timer {minutes tclcommand} {
	namespace eval [getns] {
		if {![info exists timers]} { set timers "" }
		if {![info exists timeridx]} { set timeridx 0 }
	}

	upvar [getns]::timers timers
	upvar [getns]::timeridx timeridx

	set time [expr [clock seconds] + $minutes * 60]
	incr timeridx
	set id "timer$timeridx"

	set timer [list $time $tclcommand $id]

	lappend timers $timer

	return $id
}

proc utimer {seconds tclcommand} {
	namespace eval [getns] {
		if {![info exists utimers]} { set utimers "" }
		if {![info exists utimeridx]} { set utimeridx 0 }
	}

	upvar [getns]::utimers utimers
	upvar [getns]::utimeridx utimeridx

	set time [expr [clock seconds] + $seconds]
	incr utimeridx
	set id "utimer$utimeridx"

	set timer [list $time $tclcommand $id]

	lappend utimers $timer

	return $id
}

proc timers {} {
	namespace eval [getns] {
		if {![info exists timers]} { set timers "" }
	}

	upvar [getns]::timers timers
	set temptimers ""

	foreach ut $timers {
		lappend temptimers [list [expr [lindex $ut 0] - [clock seconds]] [lindex $ut 1] [lindex $ut 2]]
	}

	return $temptimers
}

proc utimers {} {
	namespace eval [getns] {
		if {![info exists utimers]} { set utimers "" }
	}

	upvar [getns]::utimers utimers
	set temptimers ""

	foreach ut $utimers {
		lappend temptimers [list [expr [lindex $ut 0] - [clock seconds]] [lindex $ut 1] [lindex $ut 2]]
	}

	return $temptimers
}

proc killtimer {timerID} {
	namespace eval [getns] {
		if {![info exists timers]} { set timers "" }
	}

	upvar [getns]::timers timers

	set idx [lsearch $timers "* $timerID"]

	if {$idx != -1} {
		set timers [lreplace $timers $idx $idx]
	}
}

proc killutimer {timerID} {
	namespace eval [getns] {
		if {![info exists utimers]} { set utimers "" }
	}

	upvar [getns]::utimers utimers

	set idx [lsearch $utimers "* $timerID"]

	if {$idx != -1} {
		set utimers [lreplace $utimers $idx $idx]
	}
}
