/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010, 2011 Curtis Gedak
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "../include/Dialog_Partition_Copy.h"

namespace GParted
{

Dialog_Partition_Copy::Dialog_Partition_Copy( const FS & fs, Sector cylinder_size )
{
	this ->fs = fs ;
	
	BUF = cylinder_size ;
	
	Set_Resizer( false ) ;	
	Set_Confirm_Button( PASTE ) ;
}

void Dialog_Partition_Copy::Set_Data( const Partition & selected_partition, const Partition & copied_partition )
{
	this ->set_title( String::ucompose( _("Paste %1"), copied_partition .get_path() ) ) ;
	
	//set partition color
	frame_resizer_base ->set_rgb_partition_color( copied_partition .color ) ;
	
	//set some widely used values...
	MIN_SPACE_BEFORE_MB = Dialog_Base_Partition::MB_Needed_for_Boot_Record( selected_partition ) ;
	START = selected_partition .sector_start ;
	total_length = selected_partition .get_sector_length() ;
	TOTAL_MB = Utils::round( Utils::sector_to_unit( selected_partition .get_sector_length(), selected_partition .sector_size, UNIT_MIB ) ) ;
	MB_PER_PIXEL = TOTAL_MB / 500.00 ;

	//Determine minimum number of sectors needed in destination (selected) partition and
	//  handle situation where src sector size is smaller than dst sector size and an additional partial dst sector is required.
	Sector copied_min_sectors = ( copied_partition .get_byte_length() + (selected_partition .sector_size - 1) ) / selected_partition .sector_size ;

	long COPIED_LENGTH_MB = ceil( Utils::sector_to_unit( copied_min_sectors, selected_partition .sector_size, UNIT_MIB ) ) ;

	//now calculate proportional length of partition 
	frame_resizer_base ->set_x_min_space_before( Utils::round( MIN_SPACE_BEFORE_MB / MB_PER_PIXEL ) ) ;
	frame_resizer_base ->set_x_start( Utils::round(MIN_SPACE_BEFORE_MB / MB_PER_PIXEL) ) ;
	int x_end = Utils::round( (MIN_SPACE_BEFORE_MB + COPIED_LENGTH_MB) / ( TOTAL_MB/500.00 ) ) ; //> 500 px only possible with xfs...
	frame_resizer_base ->set_x_end( x_end > 500 ? 500 : x_end ) ;
	frame_resizer_base ->set_used( 
		Utils::round( Utils::sector_to_unit( 
				copied_partition .sectors_used, copied_partition .sector_size, UNIT_MIB ) / (TOTAL_MB/500.00) ) ) ;

	if ( fs .grow )
		if ( ! fs .MAX || fs .MAX > ((TOTAL_MB - MIN_SPACE_BEFORE_MB) * MEBIBYTE) )
			fs .MAX = ((TOTAL_MB - MIN_SPACE_BEFORE_MB) * MEBIBYTE) ;
		else
			fs .MAX =  fs .MAX - (BUF * selected_partition .sector_size) ;
	else
		fs .MAX = copied_partition .get_byte_length() ;

	//TODO: Since BUF is the cylinder size of the current device, the cylinder size of the copied device could differ for small disks
	if ( fs .filesystem == GParted::FS_XFS ) //bit hackisch, but most effective, since it's a unique situation
		fs .MIN = ( copied_partition .sectors_used + (BUF * 2) ) * copied_partition .sector_size;
	else
		fs .MIN = COPIED_LENGTH_MB * MEBIBYTE ;
	
	GRIP = true ;
	//set values of spinbutton_before
	spinbutton_before .set_range( MIN_SPACE_BEFORE_MB, TOTAL_MB - ceil( fs .MIN / double(MEBIBYTE) ) ) ;
	spinbutton_before .set_value( MIN_SPACE_BEFORE_MB ) ;

	//set values of spinbutton_size
	spinbutton_size .set_range( ceil( fs .MIN / double(MEBIBYTE) )
	                          , ceil( fs .MAX / double(MEBIBYTE) )
	                          ) ;
	spinbutton_size .set_value( COPIED_LENGTH_MB ) ;
	
	//set values of spinbutton_after
	spinbutton_after .set_range( 0, TOTAL_MB - MIN_SPACE_BEFORE_MB - ceil( fs .MIN / double(MEBIBYTE) ) ) ;
	spinbutton_after .set_value( TOTAL_MB - MIN_SPACE_BEFORE_MB - COPIED_LENGTH_MB ) ; 
	GRIP = false ;
	
	frame_resizer_base ->set_size_limits( Utils::round( fs .MIN / (MB_PER_PIXEL * MEBIBYTE) ),
					      Utils::round( fs .MAX / (MB_PER_PIXEL * MEBIBYTE) ) ) ;
	
	//set contents of label_minmax
	Set_MinMax_Text( ceil( fs .MIN / double(MEBIBYTE) )
	               , ceil( fs .MAX / double(MEBIBYTE) )
	               ) ;

	//set global selected_partition (see Dialog_Base_Partition::Get_New_Partition )
	this ->selected_partition = copied_partition ;
	this ->selected_partition .device_path = selected_partition .device_path ;
	this ->selected_partition .inside_extended = selected_partition .inside_extended ;
	this ->selected_partition .type = 
		selected_partition .inside_extended ? GParted::TYPE_LOGICAL : GParted::TYPE_PRIMARY ;
	//Handle situation where src sector size is smaller than dst sector size and an additional partial dst sector is required.
	this ->selected_partition .set_used(
			( (copied_partition .sectors_used * copied_partition .sector_size)
			  + (selected_partition .sector_size - 1)
			) / selected_partition .sector_size ) ;

	this ->show_all_children() ;
}

Partition Dialog_Partition_Copy::Get_New_Partition( Byte_Value sector_size )
{
	//first call baseclass to get the correct new partition
	selected_partition = Dialog_Base_Partition::Get_New_Partition( sector_size ) ;

	//set proper name and status for partition
	selected_partition .status = GParted::STAT_COPY ;

	return selected_partition ;
}

} //GParted
