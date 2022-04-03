/**
 * File              : ycPhoto.h
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 02.04.2022
 * Last Modified Date: 03.04.2022
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

#include <stdio.h>

typedef struct yc_photo_t {
	char uuid[37];
	char comment[BUFSIZ];
	void *data;
	size_t size;
} yc_photo_t;

//add photo to base - return added photo struct
yc_photo_t * yc_photo_add(
		const char * yandex_disk_token, //autorization token for Yandex Disk
		unsigned int companyId,         //yclients company id
		unsigned int clientId,          //yclients client id
		unsigned int eventId,           //yclients event id
		const char * image_path,        //path to image file
		const char * comment,           //comment string
		void * user_data,               //pointer to transfer trow callback
		int (*callback)(
			void *user_data,
			char *error
		),
		void *clientp,                  //pointer to transfer trow callback
		int (*progress_callback)(
			void *clientp,
			double dltotal,             //download total size
			double dlnow,               //download size
			double ultotal,             //upload total size
		   	double ulnow			    //upload size
		)
);

//remove photo from base
void yc_photo_remove(
		const char * yandex_disk_token, //autorization token for Yandex Disk
		unsigned int companyId,         //yclients company id
		unsigned int clientId,          //yclients client id
		unsigned int eventId,           //yclients event id
		const char * uuid,				//uuid of photo
		void * user_data,               //pointer to transfer trow callback
		int (*callback)(
			void *user_data,
			char *error
		)
);

//remove photo from base
void yc_photo_set_comment(
		const char * yandex_disk_token, //autorization token for Yandex Disk
		unsigned int companyId,         //yclients company id
		unsigned int clientId,          //yclients client id
		unsigned int eventId,           //yclients event id
		const char * uuid,				//uuid of photo
		const char * comment,			//new comment
		void * user_data,               //pointer to transfer trow callback
		int (*callback)(
			void *user_data,
			char *error
		)
);

//run callback for each photo for eventid
void yc_photo_for_each(
		const char * yandex_disk_token, //autorization token for Yandex Disk
		unsigned int companyId,         //yclients company id
		unsigned int clientId,          //yclients client id
		unsigned int eventId,           //yclients event id
		void * user_data,               //pointer to transfer trow callback
		int (*callback)(
			yc_photo_t *photo,          //photo
			void *user_data,
			char *error
		)
);
