/**
 * File              : ycPhoto.c
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 02.04.2022
 * Last Modified Date: 03.04.2022
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

#include "ycPhoto.h"
#include "cYandexDisk/cYandexDisk.h"
#include "cYandexDisk/uuid4/uuid4.h"
#include "stb/stb_image_resize.h"
#include <stdio.h>
#include <pthread.h>
#include <string.h>

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

struct yc_photo_thumbinail_data_from_jpeg_file_callback_context {
	void *data;
	size_t size;
};

void yc_photo_thumbinail_data_from_jpeg_file_callback(void *context, void *data, int size){
	struct yc_photo_thumbinail_data_from_jpeg_file_callback_context *c = context;

	size_t new_size = c->size + size;
	memcpy(c->data+c->size, data, size);
	
	c->data = realloc(c->data, new_size + BUFSIZ);
	c->size = new_size;
}

size_t yc_photo_thumbinail_data_from_jpeg_file(const char * image_path, void **_data, char **error){
	if (!_data)
		return 0;
		
	int err = 0; //error to return
	int x, y, n; //x - widhth, y - height, n - components per pixel (eg. 3 for rgb)
	
	//reading data
	unsigned char *file_data = stbi_load(image_path, &x, &y, &n, 0);

	//allocate memmory for output data
	unsigned char *output_data = MALLOC(128*128*BUFSIZ);

	//resize image
	err = stbir_resize_uint8(file_data, x, y, 0, output_data, 128, 128, 0, n);
	if (err != 1) {
		ERROR(error, "ycPhoto: can't resize image: %s\n", image_path);
		return 0;
	}

	//write image tumbinail to function
	void *data = MALLOC(BUFSIZ);
	*_data = data;
	struct yc_photo_thumbinail_data_from_jpeg_file_callback_context context;
	context.data = data;
	context.size = 0;
	
	err = stbi_write_jpg_to_func(yc_photo_thumbinail_data_from_jpeg_file_callback, &context, 128, 128, n, output_data, 100);
	if (err != 1) {
		ERROR(error, "ycPhoto: can't write thumbinail to data\n");
		return 0;
	}

	free(output_data); //no need data
	free(file_data);

	return context.size;
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
		ERROR(error, "ycPhoto: can't resize image: %s\n", image_path);
		return 1;
	}

	//write image tumbinail to JPEG file
	err = stbi_write_jpg(thumbinail_path, 128, 128, n, output_data, 100);
	if (err != 1) {
		ERROR(error, "ycPhoto: can't write thumbinail data to: %s\n", thumbinail_path);
		return 1;
	}

	free(output_data); //no need data
	free(file_data);

	return 0;
}

struct yc_photo_upload_photo_data {
	void *user_data;
	int (*callback)(void *user_data, char *error);	
};

int yc_photo_upload_photo_callback(size_t size, void *user_data, char *error)
{
	struct yc_photo_upload_photo_data *d = user_data;
	if (error)
		if (d->callback)
			d->callback(d->user_data, error);	

	free(user_data);
	return 0;
}

struct yc_photo_upload_photo_in_thread_params {
	char yandex_disk_token[128];
	int companyId;
   	int clientId;
   	int eventId;
	char image_path[BUFSIZ];
	void *thumb_data;
	size_t thumb_size;
	char uuid[37];
	char comment[BUFSIZ];
	void * user_data;
	int (*callback)(void *user_data, char *error);
	void *clientp;
	int (*progress_callback)(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
};

void *yc_photo_upload_photo_in_thread(void *_params){

	struct yc_photo_upload_photo_in_thread_params *params = _params;

	char dir_path[BUFSIZ];
	
	char *error = NULL;
	sprintf(dir_path, "app:/%u", params->companyId);
	c_yandex_disk_mkdir(params->yandex_disk_token, dir_path, &error);	
	if (error) {
		if (params->callback)
			params->callback(params->user_data, error);
		free(error);
	}

	error = NULL;
	sprintf(dir_path, "%s/%u", dir_path, params->clientId);
	c_yandex_disk_mkdir(params->yandex_disk_token, dir_path, &error);	
	if (error){
		if (params->callback)
			params->callback(params->user_data, error);	
		free(error);
	}

	error = NULL;
	sprintf(dir_path, "%s/%u", dir_path, params->eventId);
	c_yandex_disk_mkdir(params->yandex_disk_token, dir_path, &error);	
	if (error){
		if (params->callback)
			params->callback(params->user_data, error);	
		free(error);
	}	

	error = NULL;
	sprintf(dir_path, "%s/%s", dir_path, params->uuid);
	c_yandex_disk_mkdir(params->yandex_disk_token, dir_path, &error);	
	if (error){
		if (params->callback)
			params->callback(params->user_data, error);	
		free(error);
	}	
	
	char *paths[] = {"image", "thumb"};
	for (int i = 0; i < 2; ++i) {

		//set data for callback
		struct yc_photo_upload_photo_data *data = NEW(struct yc_photo_upload_photo_data); //allocate memory as run in thread
		data->user_data = params->user_data;
		data->callback = params->callback;
		
		//set image path
		char path[BUFSIZ];
		sprintf(path, "%s/%s.jpeg", dir_path, paths[i]);

		//upload image
		if (i == 0)
			c_yandex_disk_upload_file(params->yandex_disk_token, params->image_path, path, data, yc_photo_upload_photo_callback, params->clientp, params->progress_callback);
		
		if (i == 1)
			c_yandex_disk_upload_data(params->yandex_disk_token, params->thumb_data, params->thumb_size, path, data, yc_photo_upload_photo_callback, params->clientp, params->progress_callback);
	}

	//upload comment
	char path[BUFSIZ];
	sprintf(path, "%s/comment.txt", dir_path);	
	struct yc_photo_upload_photo_data *data = NEW(struct yc_photo_upload_photo_data); //allocate memory as run in thread
	data->user_data = params->user_data;
	data->callback = params->callback;	
	c_yandex_disk_upload_data(params->yandex_disk_token, (void *)params->comment, strlen(params->comment), path, data, yc_photo_upload_photo_callback, params->clientp, params->progress_callback);	

	free(params);
	pthread_exit(0);	
}

yc_photo_t * 
yc_photo_add(
		const char * yandex_disk_token, 
		unsigned int companyId, 
		unsigned int clientId, 
		unsigned int eventId, 
		const char * image_path, 
		const char * comment, 
		void * user_data, 
		int (*callback)(void *user_data, char *error), 
		void *clientp, 
		int (*progress_callback)(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow))
{
	//create uuid
	char *uuid = MALLOC(37);
	UUID4_STATE_T state; UUID4_T identifier;
	uuid4_seed(&state);
	uuid4_gen(&state, &identifier);
	if (!uuid4_to_s(identifier, uuid, 37)){
		if (callback)
			callback(user_data, "ycPhoto: Can't genarate UUID");
		return NULL;
	}

	//create thumbinail
	char *error = NULL;
	void *thumb_data;
	int thumb_size = yc_photo_thumbinail_data_from_jpeg_file(image_path, &thumb_data, &error);
	if (error) {
		if (callback)
			callback(user_data, error);
		return NULL;
	}

	//upload file in new thread
	pthread_t tid; //идентификатор потока
	pthread_attr_t attr; //атрибуты потока

	//получаем дефолтные значения атрибутов
	int err = pthread_attr_init(&attr);
	if (err) {
		if (callback)
			callback(user_data, STR("ycPhoto: Can't init THREAD attributes\n"));
		return NULL;
	}	

	//set params
	struct yc_photo_upload_photo_in_thread_params *params = NEW(struct yc_photo_upload_photo_in_thread_params);
	STRCOPY(params->yandex_disk_token , yandex_disk_token);
	params->companyId = companyId;
	params->clientId = clientId;
	params->eventId = eventId;
	STRCOPY(params->image_path , image_path);
	params->thumb_data = thumb_data;
	params->thumb_size = thumb_size;
	STRCOPY(params->uuid, uuid);
	STRCOPY(params->comment, comment);
	params->user_data = user_data;
	params->callback = callback;
	params->clientp = clientp;
	params->progress_callback = progress_callback;

	//создаем новый поток
	err = pthread_create(&tid,&attr, yc_photo_upload_photo_in_thread, params);
	if (err) {
		if (callback)
			callback(user_data, STR("ycPhoto: Can't create THREAD\n"));
		return NULL;
	}

	yc_photo_t *photo = NEW(yc_photo_t);
	photo->data = thumb_data;
	photo->size = thumb_size;
	STRCOPY(photo->uuid , uuid);
	STRCOPY(photo->comment , comment);
	
	return photo;
}

void 
yc_photo_remove(
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
)
{
	char dir_path[BUFSIZ];
	
	sprintf(dir_path, "app:/%u/%u/%u/%s", companyId, clientId, eventId, uuid);
	
	char *error = NULL;
	
	c_yandex_disk_rm(yandex_disk_token, dir_path, &error);	
	if (error) {
		if (callback)
			callback(user_data, error);
		free(error);
	}

}

struct yc_photo_set_comment_callback_t {
	void *user_data;
	int (*callback)(void *user_data, char *error);
};

int 
yc_photo_set_comment_callback(size_t size, void *user_data, char *error)
{
	struct yc_photo_set_comment_callback_t *t = user_data;
	if (error) {
		if (t->callback)
			t->callback(t->user_data, error);
		free(error);
	}

	free(t);

	return 0;
}

void 
yc_photo_set_comment(
		const char * yandex_disk_token, //autorization token for Yandex Disk
		unsigned int companyId,         //yclients company id
		unsigned int clientId,          //yclients client id
		unsigned int eventId,           //yclients event id
		const char * uuid,				//uuid of photo
		const char * comment,				
		void * user_data,               //pointer to transfer trow callback
		int (*callback)(
			void *user_data,
			char *error
		)
)
{
	char dir_path[BUFSIZ];
	
	sprintf(dir_path, "app:/%u/%u/%u/%s/comment.txt", companyId, clientId, eventId, uuid);
	
	char *error = NULL;
	
	c_yandex_disk_rm(yandex_disk_token, dir_path, &error);	
	if (error) {
		if (callback)
			callback(user_data, error);
		free(error);
	}

	struct yc_photo_set_comment_callback_t *t = NEW(struct yc_photo_set_comment_callback_t);
	t->callback = callback;
	t->user_data = user_data;
	c_yandex_disk_upload_data(yandex_disk_token, (void *)comment, strlen(comment), dir_path, t, yc_photo_set_comment_callback, NULL, NULL);
}


struct yc_photo_list_callback_data {
	void *user_data;
	int (*callback)(yc_photo_t *photo, void *user_data, char *error);
	const char * token;
	yc_photo_t *photo;
};

int yc_photo_list_download_thumb_callback(size_t size, void *data, void *user_data, char *error){
	struct yc_photo_list_callback_data *d = user_data;	
	if (size) {
		d->photo->data = data;
		d->photo->size = size;
		if (d->callback)
			d->callback(d->photo, d->user_data, NULL);
	}

	return 0;
}

int yc_photo_list_download_comment_callback(size_t size, void *data, void *user_data, char *error){
	struct yc_photo_list_callback_data *d = user_data;	
	if (size) {
		strncpy(d->photo->comment, (char *)data, size);
	}

	return 0;
}

int yc_photo_list_callback(c_yd_file_t *file, void *user_data, char *error)
{
	struct yc_photo_list_callback_data *d = user_data;	
	if (error)
		if (d->callback)
			d->callback(NULL, d->user_data, error);

	if (file) {
		
		yc_photo_t *photo = NEW(yc_photo_t);
		STRCOPY(photo->uuid, file->name);
		d->photo = photo;
		
		char path[BUFSIZ];
		
		//copy thumbinail
		sprintf(path, "%s/thumb.jpeg", file->path);
		c_yandex_disk_download_data(d->token, path, user_data, yc_photo_list_download_thumb_callback, NULL, NULL);
		
		//copy comment
		sprintf(path, "%s/comment.txt", file->path);
		c_yandex_disk_download_data(d->token, path, user_data, yc_photo_list_download_comment_callback, NULL, NULL);

	}

	return 0;
}

void 
yc_photo_for_each(
		const char * yandex_disk_token, //autorization token for Yandex Disk
		unsigned int companyId,         //yclients company id
		unsigned int clientId,          //yclients client id
		unsigned int eventId,           //yclients event id
		void * user_data,               //pointer to transfer trow callback
		int (*callback)(
			yc_photo_t *photo,		    //photo
			void *user_data,
			char *error
		)
)
{
	char dir_path[BUFSIZ];
	sprintf(dir_path, "app:/%u/%u/%u", companyId, clientId, eventId);
	
	struct yc_photo_list_callback_data data;
	data.user_data = user_data;
	data.callback = callback;
	data.token = yandex_disk_token;
	
	c_yandex_disk_ls(yandex_disk_token, dir_path, &data, yc_photo_list_callback);
}
