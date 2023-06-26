/*
LINUX:
gcc -c cJSON.c
ar rcs libcjson.a cJSON.o
gcc ino_validation.c -o ino_validation -L. -lcjson -static
gcc ino_validation.c -o ino_validation -L. -L/Library/Developer/CommandLineTools/usr/lib/swift_static/macosx/ -lcjson -Bstatic

TODO:
1. error handler for void func
2. SDK == 1 check
3. 2 modelSelect() handler
---------------------------------
Jul
1. customized file location check
2. Audio NN
*/
#define _GNU_SOURCE

#ifdef _WIN32
#include <io.h>
#include "dirent.h"	// https://codeyarns.com/tech/2014-06-06-how-to-use-dirent-h-with-visual-studio.html#gsc.tab=0
#define F_OK 0
#else // #elif __linux__
#include <inttypes.h>
#include <unistd.h>
#include <dirent.h>
#define __int64 int64_t
#define _close close
#define _read read
#define _lseek64 lseek64
#define _O_RDONLY O_RDONLY
#define _open open
#define _lseeki64 lseek64
#define _lseek lseek
#define _access access
#define stricmp strcasecmp
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "cJSON.h"
#include <locale.h>
#include <time.h>

#define PRINT_DEBUG		1
#define MAX_PATH_LENGTH 1024

/* Declear function headers */
int dirExists(const char* directory_path);
/* Return folder names under current directory */
const char* dirName(const char* directory_path);
/* Function returns nuber of content under this path */
int isDirExists(const char* path);
/* Functions below are for txt f_model manipulation */
int endsWith(const char* str, const char* suffix);
/* Returns example f_model path inside the temp JSON f_model */
const char* pathTempJSON(const char* directory_path, const char* ext, const char* key);
/* Load JSON f_model from the directory and parse into cJSON data format */
cJSON* loadJSONFile(const char* directory_path);
/* Remove char c from string str */
void removeChar(char* str, char c);
/* Validate example in directory_path and returns example path */
const char* validateINO(const char* directory_path);
/* Clear all content in the ino_validation.txt file */
void resetTXT(const char* directory_path);
/* Update content in the input to TXT f_model */
void updateTXT(const char* input);
/* Similar function as REGEX*/
void extractParam(char* line, char* param);
/* Extract header from quote symbols*/
void extractString(char* source, char* result);
/* Handler to print error message in the Arduino IDE */
void error_handler(const char* message);
/* Convert model input type to model name*/
const char* input2model(const char* input);
/* Find the dir name of input source file path*/
void extractRootDirectory(char* source, char* result);
/* Convert model input to model filename as defined in json */
const char* input2filename(const char* directory_path, const char* key);

// -------------------------------------------------------------

/* Convert model input type to model header f_model*/
const char* input2header(const char* input);

void validateJSON(void);
void resetJSON(const char* input);
void updateJSON(const char* input);
/* Function to update JSON */
void writeJSON(const char* f_path);
bool dupCheckJSON(const char* input);

char* dspFileProp(const char* f_name);

void renameFile(char* filename, int type);

void backupModel(char* input, char* sktech_path);

void rvtModel(const char* input);


/* Declear global vairables */
const char* key_amb_NN					= "modelSelect";
const char* key_amb_bypassNN1			= " .modelSelect";
const char* key_amb_bypassNN2			= " modelSelect";
const char* key_amb_VOE					= "configVideoChannel";
const char* key_amb_bypassVOE1			= " .configVideoChannel";
const char* key_amb_bypassVOE2			= " configVideoChannel";
const char* key_amb_header				= "#include";
const char* key_amb_customized			= "CUSTOMIZED";
const char* key_json					= "build";
const char* key_amb						= "Arduino15";
const char* key_ino						= ".ino";
const char* ext_json					= ".json";
const char* key_amb_default				= "DEFAULT";
const char* key_amb_default_backup		= "Dbackup";
const char* key_amb_customized_backup	= "Cbackup";
const char* fname_txt					= "ino_validation.txt";
char dir_example[100] = "NA";

/* Declear common file paths */
#ifdef _WIN32
char* path_arduino15_add = "\\AppData\\Local\\Arduino15";
char* path_ambpro2_add = "\\packages\\realtek\\hardware\\AmebaPro2\\";
char* path_model_add = "\\variants\\common_nn_models";
char* path_txtfile_add = "\\misc\\";
char* backspace = "\\";
#elif __linux__
char* path_arduino15_add = "/.arduino15";
char* path_ambpro2_add = "/packages/realtek/hardware/AmebaPro2/";
char* path_model_add = "/variants/common_nn_models";
char* path_txtfile_add = "/misc/";
char* backspace = "/";
#else
char* path_arduino15_add = "/Library/Arduino15";
char* path_ambpro2_add = "/packages/realtek/hardware/AmebaPro2/";
char* path_model_add = "/variants/common_nn_models";
char* path_txtfile_add = "/misc/";
char* backspace = "/";
#endif

const char* path_build_options_json = NULL;
const char* path_example = NULL;
const char* name_example = NULL;
char path_root[MAX_PATH_LENGTH];
char path_arduino15[MAX_PATH_LENGTH];
char path_pro2[MAX_PATH_LENGTH];
char path_model[MAX_PATH_LENGTH];
char dir_model[MAX_PATH_LENGTH];
char path_txt[MAX_PATH_LENGTH];

int main(int argc, char* argv[]) {

	setlocale(LC_ALL, "en_US.UTF-8");
	// Check if the number of input arguments is correct 
	if (argc != 3) {
#if PRINT_DEBUG
		printf("[Error] Incorrect number of input parameters. Expected 2 parameters.\r\n");
#endif
		exit(1);
	}

	// Retrieve the input parameters 
	const char* path_build = argv[1];
	const char* path_tools = argv[2];

	// Retrive root path
#ifdef _WIN32
	strcpy(path_root, getenv("USERPROFILE"));
	strcpy(path_arduino15, getenv("USERPROFILE"));
#else
	strcpy(path_root, getenv("HOME"));
	strcpy(path_arduino15, getenv("HOME"));
#endif

	strcat(path_arduino15, path_arduino15_add);
	strcpy(path_pro2, path_arduino15);
	strcat(path_pro2, path_ambpro2_add);
	strcpy(path_model, path_pro2);
	strcat(path_model, dirName(path_pro2));
	strcat(path_model, path_model_add);
	strcpy(path_txt, argv[2]);
	strcat(path_txt, path_txtfile_add);

	// Print the input parameters 
	printf("Parameter 1      = %s\n", path_build);
	printf("Parameter 2      = %s\n", path_tools);
	//printf("USERPROFILE      = %s\n", getenv("USERPROFILE"));
	//printf("HOMEDRIVE        = %s\n", getenv("HOMEDRIVE"));
	//printf("HOMEPATH         = %s\n", getenv("HOMEPATH"));
	printf("path_root        = %s\n", path_root);
	printf("path_arduino15   = %s\n", path_arduino15);
	printf("path_pro2        = %s\n", path_pro2);
	printf("ver_pro2         = %s\n", dirName(path_pro2));
	printf("path_model       = %s\n", path_model);
	printf("path_txt         = %s\n", path_txt);

	resetJSON(path_model);		//resetTXT(path_txt);
	printf("[%s][INFO] resetJSON done\n", __func__);
	path_build_options_json = pathTempJSON(path_build, ext_json, key_json);
	path_example = validateINO(path_build);
	printf("[%s][INFO] path_example            = %s\n", __func__, path_example);
	writeJSON(path_example);	//writeTXT(path_example);
	return 0;
}



const char* input2filename(const char* directory_path, const char* key) {
#if PRINT_DEBUG
	printf("[%s] open the file %s \n", __func__, directory_path);
#endif
	FILE* file = fopen(directory_path, "rb");
	if (file == NULL) {
		printf("[%s][Error] Failed to open the file.\n", __func__);
		perror("Error opening file");
		printf("Error code: %d\n", errno);
		return 1;
	}

	// Get the file size
	fseek(file, 0, SEEK_END);
	size_t file_size = ftell(file);
	fseek(file, 0, SEEK_SET);
	rewind(file);

	// Allocate memory to hold the JSON data
	char* json_data = (char*)malloc(file_size + 1);
	if (json_data == NULL) {
		printf("[%s][Error] Failed to allocate memory.\n", __func__);
		fclose(file);
		return 1;
	}

	// Read the JSON data from the file
	size_t read_size = fread(json_data, 1, file_size, file);
#if PRINT_DEBUG
	printf("[%s][Info] read_size %d\n", __func__, read_size);
	printf("[%s][Info] file_size %d\n", __func__, file_size);
#endif
	if (read_size != file_size) {
		printf("[%s][Error] Failed to read the file.\n", __func__);
		fclose(file);
		free(json_data);
		return 1;
	}
	json_data[file_size] = '\0';  // Null-terminate the string

	// Close the file
	fclose(file);

	// Parse the JSON data
	cJSON* data = cJSON_Parse(json_data);
	if (data == NULL) {
		printf("[%s][Error] Failed to parse JSON data.\n", __func__);
		return NULL;
	}
	// Print JSON data 
#if PRINT_DEBUG
	printf("[%s] json_data %s.\n", __func__, json_data);
#endif

	//const char* key = "yolov4_tiny";

	cJSON* yolov4_tiny_obj = cJSON_GetObjectItemCaseSensitive(data, key);
	if (yolov4_tiny_obj == NULL) {
		printf("Key '%s' not found.\n", key);
		cJSON_Delete(data);
		return NULL;
	}

	cJSON* file_obj = cJSON_GetObjectItemCaseSensitive(yolov4_tiny_obj, "file");
	if (file_obj == NULL) {
		printf("Attribute \"file\" not found！\n");
		cJSON_Delete(data);
		return 1;
	}

	const char* value = file_obj->valuestring;

	printf("[%s][Info] Output: %s\n", __func__, value);

	// Clean up cJSON object and allocated memory
	cJSON_Delete(data);
	free(json_data);

	return value;
}

int isDirExists(const char* path) {
	DIR* dir;
	struct dirent* ent;
	int count = 0;

	// check weather dir is valid
	if ((dir = opendir(path)) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_type == DT_DIR) {
				count++;
#if PRINT_DEBUG
				printf("[%s] Folder:%s\n", __func__, ent->d_name);
#endif
			}
			if (ent->d_type == DT_REG) {
				count++;
#if PRINT_DEBUG
				printf("[%s] File:%s\n", __func__, ent->d_name);
#endif
			}
		}
		if (count == 2) {
			/* Empty directory that contains "." and ".." only */
#if PRINT_DEBUG
			printf("[%s][Error] Empty directory found\n", __func__);
#endif
			return 0;
		}
		closedir(dir);
		return count;
	}
	else if (ENOENT == errno) {
#if PRINT_DEBUG
		printf("[%s][Error] Directory does not exist\n", __func__);
#endif
		return 0;
	}
	else {
		/* opendir() failed for some other reason. */
		return 0;
	}
}

int endsWith(const char* str, const char* suffix) {
	size_t str_len = strlen(str);
	size_t suffix_len = strlen(suffix);
	if (suffix_len > str_len) {
		return 0;
	}
	return strncmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
}

const char* input2model(const char* input) {
	const char* model_mapping[][2] = {
		{"CUSTOMIZED_YOLOV3TINY",    "yolov3_tiny"},
		{"CUSTOMIZED_YOLOV4TINY",    "yolov4_tiny"},
		{"CUSTOMIZED_YOLOV7TINY",    "yolov7_tiny"},
		{"CUSTOMIZED_MOBILEFACENET", "mobilefacenet_i16"},
		{"CUSTOMIZED_SCRFD",         "scrfd640"},
		{"DEFAULT_YOLOV3TINY",       "yolov3_tiny"},
		{"DEFAULT_YOLOV4TINY",       "yolov4_tiny"},
		{"DEFAULT_YOLOV7TINY",       "yolov7_tiny"},
		{"DEFAULT_MOBILEFACENET",    "mobilefacenet_i8"},
		{"DEFAULT_SCRFD",            "scrfd320p"}
	};

	int mapping_size = sizeof(model_mapping) / sizeof(model_mapping[0]);

	for (int i = 0; i < mapping_size; i++) {
		if (strcmp(input, model_mapping[i][0]) == 0) {
			return model_mapping[i][1];
		}
	}

	return "NA";
}

const char* input2header(const char* input) {
	const char* header = NULL;

	if (strcmp(input, "yolov3_tiny") == 0 ||
		strcmp(input, "yolov4_tiny") == 0 ||
		strcmp(input, "yolov7_tiny") == 0) {
		header = "NNObjectDetection.h";
	}
	else if (strcmp(input, "mobilefacenet_i16") == 0 ||
		strcmp(input, "scrfd640") == 0 ||
		strcmp(input, "mobilefacenet_i8") == 0 ||
		strcmp(input, "scrfd320p") == 0) {
		header = "NNFaceDetectionRecognition.h";
	}
	else if (strcmp(input, "None") == 0) {
		header = "NA";
	}

	return header;
}

int dirExists(const char* directory_path) {
	DIR* directory = opendir(directory_path);
	// check weather dir is valid
	if (directory) {
		closedir(directory);
		return 1;
	}
	else {
		printf("[%s][Error] Failed to open directory.\n", __func__);
		return 0;
	}
}

const char* dirName(const char* directory_path) {
	int sdk_counter = 0;
	struct dirent* entry;
	DIR* directory = opendir(directory_path);
	const char* sdk_name = "";
	// check dir validation
	if (directory) {
		while ((entry = readdir(directory)) != NULL) {
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
				continue;
			}
			else {
				sdk_counter++;
				sdk_name = entry->d_name;
			}
		}
		// non singular SDK validation
		if (sdk_counter > 1) {
			printf("[%s][Error] Current dirctory contains more than 1 SDK!!! \n", __func__);
			exit(1);
		}
		else {
			return sdk_name;
		}
	}
	else {
		printf("[%s][Error] Failed to open directory.\n", __func__);
	}
	closedir(directory);
}

void resetTXT(const char* directory_path) {
	DIR* dir = opendir(directory_path);
	struct stat st;

	// create directory if not exists
	if (stat(dir, &st) == -1) {
		mkdir(dir, 0700);
	}
	strcat(directory_path, fname_txt);

	// open txt file and clear everything
	FILE* file = fopen(directory_path, "w");
	if (file != NULL) {

	}
}

void updateTXT(const char* input) {
	//strcat(path_txt, fname_txt);
	FILE* file = fopen(path_txt, "a");

	if (file) {
		fprintf(file, "%s\n", input);
		fclose(file);
	}
	else {
#if PRINT_DEBUG
		printf("[%s][Error] Failed to open the file.\n", __func__);
#endif	
		perror(path_txt);
		// qqz return EXIT_FAILURE;
	}
}

const char* pathTempJSON(const char* directory_path, const char* ext, const char* key) {
	DIR* dir;
	struct dirent* ent;

#if PRINT_DEBUG
	printf("[%s][INFO] Load json file in dir \"%s\"\n", __func__, directory_path);
#endif
	if ((dir = opendir(directory_path)) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_type == DT_REG && strstr(ent->d_name, ext_json) != NULL && strstr(ent->d_name, key_json) != NULL) {
#if PRINT_DEBUG
				printf("[%s][INFO] File: %s\n", __func__, ent->d_name);
#endif
				strcat(directory_path, backspace);
				strcat(directory_path, ent->d_name);
				return directory_path;
				/*

				size_t size_file = strlen(ent->d_name);
				size_t size_json = strlen(ext);
				const char* jsonfilename = strstr(ent->d_name, key);

				if (size_file >= size_json && strcmp(ent->d_name + size_file - size_json, ext) == 0) {
					if (strlen(jsonfilename) != 0 && strlen(jsonfilename) == strlen(ent->d_name)) {
#if PRINT_DEBUG
						printf("[%s][INFO] File: %s\n", __func__, ent->d_name);
#endif
						strcat(directory_path, backspace);
						strcat(directory_path, jsonfilename);

					}
					return directory_path;
				}*/
			}
		}
	}
}

cJSON* loadJSONFile(const char* directory_path) {
	printf("[%s] open the file %s \n", __func__, directory_path);
	FILE* file = fopen(directory_path, "rb");
	if (file == NULL) {
		printf("[%s][Error] Failed to open the file.\n", __func__);
		perror("Error opening file");
		printf("Error code: %d\n", errno);
		return 1;
	}

	// Get the file size
	fseek(file, 0, SEEK_END);
	size_t file_size = ftell(file);
	fseek(file, 0, SEEK_SET);
	rewind(file);

	// Allocate memory to hold the JSON data
	char* json_data = (char*)malloc(file_size + 1);
	if (json_data == NULL) {
		printf("[%s][Error] Failed to allocate memory.\n", __func__);
		fclose(file);
		return 1;
	}

	// Read the JSON data from the file
	size_t read_size = fread(json_data, 1, file_size, file);
#if PRINT_DEBUG
	printf("[%s][Info] read_size %d\n", __func__, read_size);
	printf("[%s][Info] file_size %d\n", __func__, file_size);
#endif
	if (read_size != file_size) {
		printf("[%s][Error] Failed to read the file.\n", __func__);
		fclose(file);
		free(json_data);
		return 1;
	}
	json_data[file_size] = '\0';  // Null-terminate the string

	// Close the file
	fclose(file);

	// Parse the JSON data
	cJSON* data = cJSON_Parse(json_data);
	if (data == NULL) {
		printf("[%s][Error] Failed to parse JSON data.\n", __func__);
		return NULL;
	}
	// print JSON data 
#if PRINT_DEBUG
	printf("[%s] json_data %s.\n", __func__, json_data);
#endif

	// Clean up cJSON object and allocated memory
	//cJSON_Delete(data);
	free(json_data);

	return data;
}

void removeChar(char* str, char c) {
	int i, j;
	int len = strlen(str);
	for (i = j = 0; i < len; i++) {
		if (str[i] != c) {
			str[j++] = str[i];
		}
	}
	str[j] = '\0';
}

const char* validateINO(const char* directory_path) {
	DIR* dir;
	struct dirent* ent;

	// Open the JSON file and retrive the data
	cJSON* data = loadJSONFile(path_build_options_json);
	
	// Arduino IDE1.0 
	cJSON* path_example = cJSON_GetObjectItem(data, "sketchLocation");

	/*
	// Arduino IDE2.0
	if (strstr(path_example, key_amb) == NULL) {
		name_example = strrchr(path_example, '\\');
		//removeChar(name_example, '\\');
		//if (strstr(path_example, key_ino) == NULL && strstr(name_example, key_ino) == NULL) {
			// rename json extracted example filename
			//strcat(name_example, key_ino);
			// find filepath in includes.cache
		//}
		//printf("[%s][INFO] name_example = %s\n", __func__, name_example);
	}
	*/
#ifdef PRINT_DEBUG
	printf("[%s][INFO] Current example path: %s \n", __func__, path_example->valuestring);
#endif	

	return path_example->valuestring;
}

void extractParam(char* line, char* param) {
	char* start = strchr(line, '(');
	char* end = strchr(line, ')');
	if (start != NULL && end != NULL && end > start) {
		size_t length = end - start - 1;
		strncpy(param, start + 1, length);
		param[length] = '\0';
	}
}

void error_handler(const char* message) {
	fprintf(stderr, "[Error] %s\n", message);
	exit(1);
}

void extractString(char* source, char* result) {
	char* start = strchr(source, '\"'); // find the 1st "
	if (start == NULL) {
		strcpy(result, ""); // set as empty string if not found
		return;
	}

	start++; // skip the first "

	char* end = strchr(start, '\"'); // find the 2nd "
	if (end == NULL) {
		strcpy(result, ""); // set as empty string if not found
		return;
	}

	int length = end - start;
	strncpy(result, start, length);
	result[length] = '\0'; // add ending param at EOL
}

void extractString2(char* source, char* result) {
	char* start = strchr(source, '<'); // find the 1st <
	if (start == NULL) {
		strcpy(result, ""); // set as empty string if not found
		return;
	}

	start++; // skip the first "

	char* end = strchr(start, '>'); // find the 1st >
	if (end == NULL) {
		strcpy(result, ""); // set as empty string if not found
		return;
	}

	int length = end - start;
	strncpy(result, start, length);
	result[length] = '\0'; // add ending param at EOL
}

void extractRootDirectory(char* filepath, char* rootDir) {
#ifdef _WIN32
	char* lastBackslash = strrchr(filepath, '\\');
#else
	char* lastBackslash = strrchr(filepath, '/');
#endif
	if (lastBackslash != NULL) {
		*lastBackslash = '\0';
#if PRINT_DEBUG
		printf("[%s] Dir: %s\n", __func__, filepath);
#endif
	}
}

void resetJSON(const char* input) {
	DIR* dir;
	struct dirent* entry;

	dir = opendir(input);
	if (dir == NULL) {
		perror("Error opening directory");
		return;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (strstr(entry->d_name, ".json") != NULL) {
			char filepath[256];
			snprintf(filepath, sizeof(filepath), "%s/%s", input, entry->d_name);

			FILE* file = fopen(filepath, "r");
			if (file == NULL) {
				perror("Error opening file");
				continue;
			}

			fseek(file, 0, SEEK_END);
			long file_size = ftell(file);
			fseek(file, 0, SEEK_SET);

			char* file_contents = (char*)malloc(file_size + 1);
			fread(file_contents, file_size, 1, file);
			fclose(file);

			cJSON* root = cJSON_Parse(file_contents);
			if (root == NULL) {
				fprintf(stderr, "Error parsing JSON in file: %s\n", entry->d_name);
				free(file_contents);
				continue;
			}

			cJSON* fwfs = cJSON_GetObjectItemCaseSensitive(root, "FWFS");
			if (fwfs == NULL || !cJSON_IsObject(fwfs)) {
				cJSON_Delete(root);
				free(file_contents);
				fprintf(stderr, "Invalid JSON format in file: %s\n", entry->d_name);
				continue;
			}

			cJSON* files = cJSON_GetObjectItemCaseSensitive(fwfs, "files");
			if (files == NULL || !cJSON_IsArray(files)) {
				cJSON_Delete(root);
				free(file_contents);
				fprintf(stderr, "Invalid JSON format in file: %s\n", entry->d_name);
				continue;
			}

			cJSON_DeleteItemFromObject(fwfs, "files");

			cJSON* new_files = cJSON_CreateArray();
			cJSON_AddItemToObject(fwfs, "files", new_files);

			char* new_file_contents = cJSON_Print(root);
			cJSON_Delete(root);
			free(file_contents);

			FILE* new_file = fopen(filepath, "w");
			if (new_file == NULL) {
				perror("Error opening file for writing");
				free(new_file_contents);
				continue;
			}

			fwrite(new_file_contents, strlen(new_file_contents), 1, new_file);
			fclose(new_file);
			free(new_file_contents);

			printf("[INFO] %s has been reset\n", entry->d_name);
		}
	}

	closedir(dir);
}

char* convert_space_to_dash(const char* str) {
	char* new_str = malloc(strlen(str) + 1);
	int i = 0;
	int j = 0;
	while (str[i] != '\0') {
		if (str[i] == ' ') {
			new_str[j] = '-';
		}
		else {
			new_str[j] = str[i];
		}
		i++;
		j++;
	}
	new_str[j] = '\0';
	return new_str;
}

char* dspFileProp(const char* filename) {
	struct stat file_model_stats;
	if (stat(filename, &file_model_stats) == -1) {
		fprintf(stderr, "[Error] Default model %s does not exist. Please check %s again.\n", filename, filename);
		exit(1);
	}

	char* file_model_date = ctime(&file_model_stats.st_mtime);
	file_model_date[24] = '\0';
	//return file_model_date;
	return convert_space_to_dash(file_model_date);
}

void renameFile(char* filename, int type) {
	char dest_path[100] = "";	// Replace with the desired destination path

	if (type == 1) {			// Backup Dmodel
		char filename_modified[200] = "";
		char dsp_file_prop[100] = "";
		
		// original model filename
		strcat(path_model, backspace);
		strcat(path_model, filename);

		// Construct the modified model filename
		strcat(filename_modified, "Dbackup_");
		strcat(filename_modified, dspFileProp(path_model));
		strcat(filename_modified, "_");
		strcat(filename_modified, filename);
		printf("[%s] %s\n", __func__, filename_modified);

		char source_path[300] = "";
		strcat(source_path, dir_example);
		strcat(source_path, backspace);
		strcat(source_path, filename);
		printf("[%s] source_path %s\n", __func__, source_path);

		char destination_path[300] = "";
		extractRootDirectory(path_model, dir_model);
		strcat(destination_path, path_model);
		strcat(destination_path, backspace);
		strcat(destination_path, filename_modified);
		printf("[%s] destination_path %s\n", __func__, destination_path);

		//---------------------------------------------- TODO: copying file from 1 source path -> dest path
		if (rename(source_path, destination_path) != 0) {
			perror("Error occurred while renaming the file");
			exit(1);
		}
		
		printf("[%s][INFO] Dmodel Backup done.\n", __func__);
		exit(1);
	}
	else {					// Backup Cmodel
		char filename_modified[200] = "";
		char dsp_file_prop[100] = ""; // Replace with the appropriate dspFileProp() implementation

		// Construct the modified filename
		strcat(filename_modified, "Cbackup_");
		strcat(filename_modified, dsp_file_prop);
		strcat(filename_modified, "_");
		strcat(filename_modified, filename);

		char source_path[300] = "";
		strcat(source_path, dest_path);
		strcat(source_path, filename);

		char destination_path[300] = "";
		strcat(destination_path, dest_path);
		strcat(destination_path, filename_modified);


		if (rename(source_path, destination_path) != 0) {
			perror("Error occurred while renaming the file");
			exit(1);
		}

		printf("[INFO] Cmodel Backup done.\n");
	}
}

void backupModel(char* input, char* sktech_path) {
	DIR* dir = opendir(path_model);
	/* check whether default example has been back up */
	if (dir) {
		struct dirent* entry;
		while ((entry = readdir(dir)) != NULL) {
			if (strstr(entry->d_name, "Dbackup") != NULL) {		// customized model has been used
				printf("[%s][INFO] Backup-ed %s found !!!\n", __func__, input);
				break;
			}
		}
		closedir(dir);
	}

	renameFile(input, 1);
	
	exit(1);
	// Backup Cmodel: source sketch folder, dest viriant folder
	char source_path[300] = "";
	strcat(source_path, sktech_path);
	strcat(source_path, backspace);
	strcat(source_path, input);

	char destination_path[300] = "";
	strcat(destination_path, path_model);
	strcat(destination_path, backspace);
	strcat(destination_path, input);

	printf("source_path '%s':\n", source_path);
	printf("destination_path '%s':\n", destination_path);

	FILE* source_file = fopen(source_path, "rb");
	FILE* destination_file = fopen(destination_path, "wb");
	if (source_file && destination_file) {
		char buffer[4096];
		size_t bytes_read;

		while ((bytes_read = fread(buffer, 1, sizeof(buffer), source_file)) > 0) {
			fwrite(buffer, 1, bytes_read, destination_file);
		}

		fclose(source_file);
		fclose(destination_file);
	}
	else {
		perror("Error occurred while copying the file");
		exit(1);
	}
	
	// renameFile(input, 0);

    // Copy Cmodel
    char destination_path_copy[300] = "";
    strcat(destination_path_copy, destination_path);
    strcat(destination_path_copy, backspace);
    strcat(destination_path_copy, input);

    FILE* source_cfile = fopen(source_path, "rb");
    FILE* destination_cfile = fopen(destination_path_copy, "wb");

    if (source_cfile && destination_cfile) {
		char buffer[4096];
        size_t bytes_read;
		while ((bytes_read = fread(buffer, 1, sizeof(buffer), source_cfile)) > 0) {
			fwrite(buffer, 1, bytes_read, destination_cfile);
        }
		fclose(source_cfile);
		fclose(destination_cfile);
    } 
	else {
            perror("Error occurred while copying the file");
            exit(1);
    }

    printf("[INFO] Cmodel copied.\n");

	exit(1);
}

void writeJSON(const char* f_path) {
	DIR* dir;
	struct dirent* ent;
	const char buf[MAX_PATH_LENGTH] = "";
	const char backslash[] = "\\";
	char line[MAX_PATH_LENGTH] = { 0 };
	unsigned int line_count = 0;
	char voe_status[100] = "NA";
	char model_type[100] = "";
	char model_name_od[100] = "";
	char model_name_fd[100] = "";
	char model_name_fr[100] = "";
	char header_od[100] = "NA";
	char header_fd[100] = "NA";
	char header_fr[100] = "NA";
	char header_all[100] = "";
	char fname_od[100] = "NA";
	char fname_fd[100] = "NA";
	char fname_fr[100] = "NA";
	char line_strip_header[100] = "NA";
	char line_strip_headerNN[100] = "NA";
	

	f_path = path_example;

#if PRINT_DEBUG
	printf("[%s][INFO] Load example: \"%s\"\n", __func__, f_path);
#endif
	
	/* check path format : IDE1 file path, IDE2 dir path */ 
	if (strstr(f_path, ".ino") == NULL) {
#if PRINT_DEBUG
		printf("IDE2\n");
#endif
		DIR* dir;
		struct dirent* ent;

		// check weather dir is valid
		if ((dir = opendir(f_path)) != NULL) {
			/* print all the files and directories within directory */
			while ((ent = readdir(dir)) != NULL) {
				if (ent->d_type == DT_REG && strstr(ent->d_name, ".ino") != NULL) {
#if PRINT_DEBUG
					printf("[%s] File:%s\n", __func__, ent->d_name);
#endif
					strcat(f_path, backspace);
					strcat(f_path, ent->d_name);
					printf("[%s] path:%s\n", __func__, f_path);
				}
			}
		}
		else {
			/* opendir() failed for some other reason. */
			printf("[%s][Error] Faield to open temp dir in IDE2.0 :%s\n", __func__, f_path);
			// qqz return EXIT_FAILURE;
		}
	}
	
	FILE* f_model = fopen(f_path, "r");
	char param[100];
	if (f_model) {
		char line[MAX_PATH_LENGTH];
		while (fgets(line, sizeof(line), f_model)) {
			/* check whether keywordNN in f_model content */
			if (strstr(line, key_amb_NN) != NULL && strstr(line, "//") == NULL && strstr(line, key_amb_bypassNN1) == NULL && strstr(line, key_amb_bypassNN2) == NULL) {
				extractParam(line, param);
				printf("Extracted parameter: %s\n", param);
				char* token;
				token = strtok(param, ", ");
				if (token != NULL) {
					strcpy(model_type, token);
					printf("Model Type: %s\n", model_type);
					/* ------------------ object detection ------------------*/
					token = strtok(NULL, ", ");
					printf("1 token: %s\n", token);
					if (token != NULL) {
						// check model combination rules
						if (strcmp(model_type, "OBJECT_DETECTION") == 0) {
							if (strcmp(token, "NA_MODEL") == 0 || strstr(token, "YOLO") == NULL) {
								goto error_combination;
							}
							// check customized od model
							if (strstr(token, key_amb_customized) != NULL) {
#if PRINT_DEBUG
								printf("od key_amb_customized\n");
								printf("customized od: %s\n", input2model(token));
#endif
								extractRootDirectory(path_example, dir_example);
#if PRINT_DEBUG
								printf("customized dir: %s\n", dir_example);
#endif
								DIR* dir;
								struct dirent* ent;
								int count = 0;

								// check weather dir is valid
								if ((dir = opendir(dir_example)) != NULL) {
									/* print all the files and directories within directory */
									while ((ent = readdir(dir)) != NULL) {
										if (ent->d_type == DT_REG) {
											count++;
										}
										if (strstr(ent->d_name, ".nb") != NULL) {
											if (strstr(ent->d_name, "yolo") != NULL) {
#if PRINT_DEBUG
												printf("%s\n", ent->d_name);
#endif
											}
											else {
												goto error_customized_mismatch;
											}
										}
									}
								}
								if (count <= 1) {
									goto error_customized_missing;
								}
							}
						}
						strcpy(model_name_od, token);
						/* ----------------- face detection -----------------*/
						token = strtok(NULL, ", ");
						printf("2 token: %s\n", token);
						if (token != NULL) {
							// check model combination rules
							if (strcmp(model_type, "FACE_DETECTION") == 0) {
								if (strcmp(token, "NA_MODEL") == 0 || strstr(token, "SCRFD") == NULL) {
									goto error_combination;
								}
								// check customized fd model
								if (strstr(token, key_amb_customized) != NULL) {
									printf("-----------------------------------------\n");
									printf("  fd key_amb_customized\n");
									printf("  customized fd: %s\n", input2model(token));
									// goto path_model and open the file ends with .json
									if (dirExists(path_model)) {
										DIR* dir = opendir(path_model);
										struct dirent* entry;

										while ((entry = readdir(dir)) != NULL) {
											if (endsWith(entry->d_name, ".json")) {
												char fpath_nn_json[MAX_PATH_LENGTH];
												char dir_nn_json[MAX_PATH_LENGTH];

												sprintf(fpath_nn_json, "%s\\%s", path_model, entry->d_name);
												cJSON* fname_model = input2filename(fpath_nn_json, input2model(token));
												strcpy(fname_od, fname_model);
											}
										}
									}
									extractRootDirectory(path_example, dir_example);
									strcpy(dir_example, path_example);

									DIR* dir;
									struct dirent* ent;
									int count = 0;

									/* check weather example directory is valid */ 
									if ((dir = opendir(path_example)) != NULL) {
										/* print all the files and directories within directory */
										while ((ent = readdir(dir)) != NULL) {
											if (ent->d_type == DT_REG) {
												count++;
											}
											/* check model file (.nb) within example directory */
											if (strstr(ent->d_name, ".nb") != NULL) {
												if (strstr(ent->d_name, "scrfd") != NULL) {		// model naming convension check
#if PRINT_DEBUG
													printf("[%s] model fname %s\n", __func__, ent->d_name);
													printf("[%s] json fname %s\n", __func__, fname_od);
#endif
													if (strcmp(ent->d_name, fname_od) != 0) {	// exampel file name does not match in json
														goto error_customized_mismatch;
													}
													else {	
#if PRINT_DEBUG
														printf("[%s][Info] Found customized model %s\n", __func__, ent->d_name);
#endif
														backupModel(ent->d_name, dir_example);
														// check when to revertModel()	
													}
													exit(1);
												}
												else {
													goto error_customized_mismatch;
												}
											}
										}
									}
									printf("[%s] %d\n", __func__, count);
									if (count <= 1) {
										goto error_customized_missing;
									}
								}
							}
							strcpy(model_name_fd, token);
							/*-------------- face recognition --------------*/
							token = strtok(NULL, ", ");
							printf("3 token: %s\n", token);
							// check model combination rules
							if (strcmp(model_type, "FACE_RECOGNITION") == 0) {
								if (strcmp(model_name_fd, "NA_MODEL") == 0 || strstr(model_name_fd, "SCRFD") == NULL || strcmp(token, "NA_MODEL") == 0 || strstr(token, "MOBILEFACENET") == NULL) {
									goto error_combination;
								}
								// check customized fr model
								if (strstr(token, key_amb_customized) != NULL) {
									printf("fr key_amb_customized\n");
									printf("customized fr: %s\n", input2model(token));
									extractRootDirectory(path_example, dir_example);
									printf("customized dir: %s\n", dir_example);

									DIR* dir;
									struct dirent* ent;
									int count = 0;

									// check weather dir is valid
									if ((dir = opendir(dir_example)) != NULL) {
										/* print all the files and directories within directory */
										while ((ent = readdir(dir)) != NULL) {
											if (ent->d_type == DT_REG) {
												count++;
											}
											if (strstr(ent->d_name, ".nb") != NULL) {
												if (strstr(ent->d_name, "mobilefacenet") != NULL) {
#if PRINT_DEBUG
													printf("%s\n", ent->d_name);
#endif
												}
												else {
													goto error_customized_mismatch;
												}
											}
										}
									}
									if (count <= 1) {
										goto error_customized_missing;
									}
								}
							}
							if (token != NULL) {
								strcpy(model_name_fr, token);
							}
						}
					}

					/* default settings for all models */
					else {
						/* provide default settings for all models if user never provide any selections*/
						if (strcmp(model_type, "OBJECT_DETECTION") == 0 && strcmp(input2model(model_name_od), "NA") == 0) {
							printf("111\n");
							strcpy(model_name_od, "DEFAULT_YOLOV4TINY");
							strcpy(model_name_fd, "NA_MODEL");
							strcpy(model_name_fr, "NA_MODEL");
						}
						if (strcmp(model_type, "FACE_DETECTION") == 0 && strcmp(input2model(model_name_od), "NA") == 0) {
							printf("222\n");
							strcpy(model_name_od, "NA_MODEL");
							strcpy(model_name_fd, "DEFAULT_SCRFD");
							strcpy(model_name_fr, "NA_MODEL");
						}
						if (strcmp(model_type, "FACE_RECOGNITION") == 0 && strcmp(input2model(model_name_od), "NA") == 0) {
							printf("333\n");
							strcpy(model_name_od, "NA_MODEL");
							strcpy(model_name_fd, "DEFAULT_SCRFD");
							strcpy(model_name_fr, "DEFAULT_MOBILEFACENET");
						}
					}
				}
				fclose(f_model);
				printf("-------------------------------------\n");
				printf("Model Name OD: %s\n", input2model(model_name_od));
				printf("Model Name FD: %s\n", input2model(model_name_fd));
				printf("Model Name FR: %s\n", input2model(model_name_fr));
				printf("-------------------------------------\n");
				//updateTXT(input2model(model_name_od));
				//updateTXT(input2model(model_name_fd));
				//updateTXT(input2model(model_name_fr));

				// ---------------------------------------
				// default add in at least one model for NN examples
			}
		}
	}
	return 0;

error_combination:
	error_handler("Model combination mismatch. Please check modelSelect() again.");

error_customized_missing:
	error_handler("Model missing. Please check your sketch folder again.");

error_customized_mismatch:
	error_handler("Customized model mismatch. Please check your sketch folder again.");
}