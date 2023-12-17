
#include "util.hpp"

#include <cstdio>
#include <chrono>
#include <thread>

#include "imgui.h"


#define WUFFS_IMPLEMENTATION
#include "wuffs.h"
#undef WUFFS_IMPLEMENTATION


std::vector<uint8_t> load_file(std::string filename, size_t offset, size_t len, int retries) {
	 FILE* file;
	 std::vector<uint8_t> contents;

	#ifdef _WIN32
	 errno_t error = fopen_s(&file, filename.c_str(), "rb");

	 if (error != 0) {
		 fprintf(stderr, "Failed to open file %s.\n", filename.c_str());

		 std::this_thread::sleep_for(std::chrono::milliseconds(2));
		 
		 if (retries <= 0)
			return contents;

		 return load_file(filename, offset, len, retries - 1);
	 }
	#else
	 file = fopen(filename.c_str(), "r");
	 if (!file) {
		 fprintf(stderr, "Failed to open file %s.\n", filename.c_str());
		 return contents;
	 }
	#endif
	 fseek(file, 0, SEEK_END);
	 size_t filesize = ftell(file);
	 rewind(file);

	 if (len == -1) len = filesize;

	 contents.resize(len + 1);

	 fseek(file, offset, SEEK_CUR);

	 fread(contents.data(), 1, len, file);
	 fclose(file);

	 contents[len] = '\0';

	 return contents;
}

std::string gl_error_name(uint32_t error_code) {
	switch (error_code) {
	case GL_NO_ERROR:
		return "no error.";
	case GL_INVALID_ENUM:
		return "GL_INVALID_ENUM";
	case GL_INVALID_VALUE:
		return "GL_INVALID_VALUE";
	case GL_INVALID_OPERATION:
		return "GL_INVALID_OPERATION";
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		return "GL_INVALID_FRAMEBUFFER_OPERATION";
	case GL_OUT_OF_MEMORY:
		return "GL_OUT_OF_MEMORY";
	case GL_STACK_UNDERFLOW:
		return "GL_STACK_UNDERFLOW";
	case GL_STACK_OVERFLOW:
		return "GL_STACK_OVERFLOW";
	default:
		return "UNKNOWN ERROR!";
	}
}



// SECTION: Some basic string parsing functions, based on iterating a const char*


// This section contains some basic parsing functions, probably should replace this with something better later!!
// Skip until the next non-whitespace character
void skip_whitespace(const char*& it) {
	while (it && *it && std::isspace(*it)) it++;
}


void skip_whitespace_not_nl(const char*& it) {
	while (it && *it && std::isspace(*it) && *it != '\n' && *it != '\r') it++;
}



void skip_until_whitespace(const char*& it) {
	while (it && *it && !std::isspace(*it)) it++;
}


std::string get_next_token(const char*& it) {
	const char* it2 = it;
	while (it2 && *it2 && !std::isspace(*it2) && *it2 != ';') it2++;
	return std::string(it, it2);
}


std::string consume_next_token(const char*& it) {
	const char* it2 = it;
	while (it && *it && !std::isspace(*it) && *it != ';') it++;
	return std::string(it2, it);
}


std::string consume_string(const char*& it) {
	consume_token(it, "\"");
	const char* start = it;
	while (it && *it && *it != '\"') it++;
	return std::string(start, it++);
}


// Skip until the next newline character
void skip_to_newline(const char*& it) {
	while (it && *it && *it != '\n') it++;
}


bool check_token(const char*& it, const std::string& str) {
	if (strncmp(it, str.c_str(), str.length()) == 0) {
		return true;
	}
	return false;
}



bool consume_token(const char*& it, const std::string& str) {
	if (check_token(it, str)) {
		it += str.length();
		return true;
	}
	return false;
}


bool consume_glsl_type(const char*& it) {
	return
		consume_token(it, "bool") ||
		consume_token(it, "int") ||
		consume_token(it, "uint") ||
		consume_token(it, "float") ||
		consume_token(it, "double") ||

		consume_token(it, "vec2") ||
		consume_token(it, "vec3") ||
		consume_token(it, "vec4");
}


float consume_float(const char*& it) {
	// Read a float from a string of characters
	char* end;
	float n = strtof(it, &end);
	it = end;
	return n;
}

int consume_int(const char*& it) {
	// Read a float from a string of characters
	char* end;
	int n = strtol(it, &end, 10);
	it = end;
	return n;
}


bool maybe_consume_float(const char*& it, float& f) {
	char* end;
	f = strtof(it, &end);

	if (end == it) {
		return false;
	}
	else {
		it = end;
		return true;
	}
}


bool maybe_consume_int(const char*& it, int& i) {
	char* end;
	i = strtol(it, &end, 10);

	if (end == it) {
		return false;
	}
	else {
		it = end;
		return true;
	}
}





namespace ImGui {
	Tag::Tag(const char* str_id) {
		PushID(str_id);
	}

	Tag::Tag(const char* str_id_begin, const char* str_id_end) {
		PushID(str_id_begin, str_id_end);
	}
	
	Tag::Tag(const void* ptr_id) {
		PushID(ptr_id);
	}

	Tag::Tag(int int_id) {
		PushID(int_id);
	}

	Tag::~Tag() {
		PopID();
	}
}
