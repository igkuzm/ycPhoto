/**
 * File              : ycPhoto.c
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 02.04.2022
 * Last Modified Date: 02.04.2022
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

#include "ycPhoto.h"
#include "stb/stb_image_resize.h"

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


int yc_photo_thumbinail_from_jpeg_file(const char *filename, const char *thumbinail_filename, char **error){
	int err = 0; //error to return
	int x, y, n; //x - widhth, y - height, n - components per pixel (eg. 3 for rgb)
	
	//reading data
	unsigned char *file_data = stbi_load(filename, &x, &y, &n, 0);

	//allocate memmory for output data
	void *output_data_ptr =	malloc(128*128*BUFSIZ);
	if (output_data_ptr == NULL) { //check of allocating memory
		perror("Malloc output_data_ptr");
		exit(EXIT_FAILURE);	
	}
	unsigned char *output_data = (unsigned char *)output_data_ptr; //assign data 

	//resize image
	err = stbir_resize_uint8(file_data, x, y, 0, output_data, 128, 128, 0, n);
	if (err != 1) {
		ERROR(error, "ycPhoto ERROR: can't resize image: %s\n", filename);
		return 1;
	}

	//write image tumbinail to JPEG file
	err = stbi_write_jpg(thumbinail_filename, 128, 128, n, output_data, 100);
	if (err != 1) {
		ERROR(error, "ycPhoto ERROR: can't write thumbinail data\n");
		return 1;
	}

	free(output_data); //no need data
	free(file_data);

	return 0;
}
