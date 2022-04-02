/**
 * File              : ycPhoto.c
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 02.04.2022
 * Last Modified Date: 02.04.2022
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

#include "ycPhoto.h"
#include "cYandexDisk/cYandexDisk.h"
#include "stb/stb_image_resize.h"
#include <stdio.h>

#define STBIW_ASSERT(x)
#define STBIR_ASSERT(x)

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize.h"

//memory allocation helpers
#define MALLOC(size) ({void* const ___p = malloc(size); if(!___p) {perror("Malloc"); exit(EXIT_FAILURE);} ___p;})
#define REALLOC(ptr, size)	({ void* const ___s = ptr; void* const ___p = realloc(___s, size);	if(!___p) { perror("Realloc"); exit(EXIT_FAILURE); } ___p; })
#define NEW(T) ((T*)MALLOC(sizeof(T)))

//error and string helpers
#define ERROR(ptr, ...) ({if(ptr) {*ptr = MALLOC(BUFSIZ); sprintf(*ptr, __VA_ARGS__);};})
#define STR(...) ({char ___str[BUFSIZ]; sprintf(___str, __VA_ARGS__); ___str;})
#define STRCOPY(str0, str1) ({size_t ___size = sizeof(str0); strncpy(str0, str1, ___size - 1); str0[___size - 1] = '\0';})

static void * yc_photo_upload_progress_data = NULL;
static int yc_photo_upload_progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
	return 0;
}

int yc_photo_thumbinail_from_jpeg_file(const char * image_path, const char * thumbinail_path, char **error){
	int err = 0; //error to return
	int x, y, n; //x - widhth, y - height, n - components per pixel (eg. 3 for rgb)
	
	//reading data
	unsigned char *file_data = stbi_load(image_path, &x, &y, &n, 0);

	//allocate memmory for output data
	unsigned char *output_data = MALLOC(128*128*BUFSIZ);

	//resize image
	err = stbir_resize_uint8(file_data, x, y, 0, output_data, 128, 128, 0, n);
	if (err != 1) {
		ERROR(error, "ycPhoto ERROR: can't resize image: %s\n", image_path);
		return 1;
	}

	//write image tumbinail to JPEG file
	err = stbi_write_jpg(thumbinail_path, 128, 128, n, output_data, 100);
	if (err != 1) {
		ERROR(error, "ycPhoto ERROR: can't write thumbinail data to: %s\n", thumbinail_path);
		return 1;
	}

	free(output_data); //no need data
	free(file_data);

	return 0;
}

typedef struct yc_photo_upload_photo_data {
	void *user_data;
	int (*callback)(void *user_data, char *error);	
	char comment[BUFSIZ];
	char token[BUFSIZ];
	char path[BUFSIZ];
} yc_photo_upload_photo_data;

int yc_photo_upload_photo_callback(size_t size, void *user_data, char *error)
{
	struct yc_photo_upload_photo_data *data = user_data;
	if (error)
		data->callback(data->user_data, error);	

	char *_error = NULL;
	c_yandex_disk_patch(data->token, data->path, data->comment, &_error);
	if (_error)
		data->callback(data->user_data, _error);	

	free(user_data);
	return 0;
}

int yc_photo_upload_photo(const char * yandex_disk_token, long companyId, int clientId, int eventId, const char * image_path, const char * thumbinail_path, const char * uuid, const char * comment, void * user_data, int (*callback)(void *user_data, char *error))
{

	//create thumbinail
	char *error = NULL;
	yc_photo_thumbinail_from_jpeg_file(image_path, thumbinail_path, &error);
	if (error) {
		callback(user_data, error);
		return -1;
	}
	
	//set image comment
	char comment_json[BUFSIZ];
	sprintf(comment_json, "{\"comment_ids\":{\"private_resource\":\"%s\", \"public_resource\":\"%s\"}}", comment, comment);
		
	char *paths[] = {"photo", "thumbinails"};
	for (int i = 0; i < 2; ++i) {
		char *error = NULL;
		//create directory
		char path[BUFSIZ];
		sprintf(path, "app:/%ld_%d_%d_%s", companyId, clientId, eventId, paths[i]);
		c_yandex_disk_mkdir(yandex_disk_token, path, &error);
		if (error)
			callback(user_data, error);
	
		//set data for callback
		struct yc_photo_upload_photo_data *data = NEW(yc_photo_upload_photo_data); //allocate memory as run in thread
		data->user_data = user_data;
		data->callback = callback;
		STRCOPY(data->comment, comment_json);
		STRCOPY(data->token, yandex_disk_token);
		
		//set image path
		sprintf(path, "%s/%s.jpeg", path, uuid);
		STRCOPY(data->path, path);

		const char *image = image_path;
		if (i == 1) image = thumbinail_path;
		c_yandex_disk_upload_file(yandex_disk_token, image, path, data, yc_photo_upload_photo_callback, yc_photo_upload_progress_data, yc_photo_upload_progress_callback);
	}

	return 0;
}
