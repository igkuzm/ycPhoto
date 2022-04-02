/**
 * File              : ycPhoto.h
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 02.04.2022
 * Last Modified Date: 02.04.2022
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

#include <stdio.h>

//make thumbinail from jpeg image
int yc_photo_thumbinail_from_jpeg_file(
		const char *filename,				//path to jpeg image
		const char *thumbinail_filename,    //path to thumbinail - were to write
		char **error                        //pointer to error
);
