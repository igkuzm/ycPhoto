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
		const char *image_path,				//path to jpeg image
		const char *thumbinail_path,        //path to thumbinail - were to write
		char **error                        //pointer to error
);

int yc_photo_upload_photo(const char * yandex_disk_token, long companyId, int clientId, int eventId, const char * image_path, const char * thumbinail_path, const char * uuid, const char * comment, void * user_data, int (*callback)(void *user_data, char *error));
