﻿
#include "shader.hpp"
#include "glad/gl.h"
#include "imgui.h"

#include <vector>
#include <iostream>
#include <mutex>
#include <optional>

#include "../util.hpp"

// Utility function to compile a GL shader from std::string
// TODO (High): Error handling here!
static uint32_t compile_gl_shader(GLenum type, const std::string src) {
	uint32_t shader = glCreateShader(type);
	const char* src_c = src.data();
	int32_t length = src.length();
	glShaderSource(shader, 1, &src_c, &length);

	glCompileShader(shader);

	return shader;
}


Shader::Shader() : IAsset("Shader") {

}

Shader::~Shader() {

}

void Shader::load_from_file(const char* filename) {
	std::vector<uint8_t> src = load_file(filename);
	load_from_string((const char*)src.data());
}

void Shader::reload() {
	should_reload = true;
}

// So al code paths can return the same type
template<typename T>
std::optional<T> dummy_consume() {
	return {};
}


// Should never be!
template<typename T>
std::optional<T> consume_directive_argument(const char*& it) {
	return {};
}

template<>
std::optional<float> consume_directive_argument<float>(const char*& it) {
	float f = 0;
	if (!maybe_consume_float(it, f)) return {};
	consume_token(it, ",");
	skip_whitespace(it);
	return f;
}


template<>
std::optional<int> consume_directive_argument<int>(const char*& it) {
	int i = 0;
	if (!maybe_consume_int(it, i)) return {};
	consume_token(it, ",");
	skip_whitespace(it);
	return i;
}



bool consume_directive(const char*& it, std::string name, std::vector<const char*>& sources, std::vector<int32_t>& lengths) {
	const char* it2 = it;

	if (!consume_token(it2, "#")) return false;
	if (!consume_token(it2, name)) return false;

	skip_to_newline(it2);

	lengths.push_back(it - sources[sources.size() - 1]);
	sources.push_back(it2);

	it = it2;

	return true;
}


template<typename ...T>
auto consume_directive(const char*& it, std::string name, std::vector<const char*>& sources, std::vector<int32_t>& lengths) {
	const char* it2 = it;

	auto fail_return = std::pair{ false, std::make_tuple(dummy_consume<T>()...) };

	if (!consume_token(it2, "#")) return fail_return;
	if (!consume_token(it2, name)) return fail_return;

	if (!consume_token(it2, "(")) return fail_return;

	auto paramters = std::tuple{ consume_directive_argument<T>(it2)... };

	if (!consume_token(it2, ")")) return fail_return;
	skip_to_newline(it2);

	lengths.push_back(it - sources[sources.size() - 1]);
	sources.push_back(it2);

	it = it2;

	return std::pair(true, paramters);
}


std::string get_next_uniform_name(const  char* it) {
	while (consume_token(it, "#")) skip_to_newline(it);
	skip_whitespace(it);
	consume_token(it, "uniform");
	skip_whitespace(it);
	consume_glsl_type(it);
	skip_whitespace(it);

	return get_next_token(it);
}


// Utility function to load a shaders source
// it should be a pointer to the first character in the string after a #type directive
uint32_t Shader::load_shader_src(GLenum type, const char* it, int& line) {
	// Find the next #directive
	std::vector<const char*> sources = {"#version 450\n"};
	std::vector<int32_t> lengths = {13};

	int line_offset = line;

	skip_whitespace(it);
	sources.push_back(it);

	while (*it) {
		if (*it == '#') {
			auto [is_range_directive, range_params] = consume_directive<float, float, float>(it, "range", sources, lengths);

			if (is_range_directive) {
				auto& [min, max, step] = range_params;

				if (!min || !max) fprintf(stderr, "Error parsing shader directive #range. Usage: #range(min, max, {step})");

				std::string uniform_name = get_next_uniform_name(it);

				ranges[uniform_name] = std::pair(*min, *max);

				if (step) {
					steps[uniform_name] = *step;
				}
			}

			auto [is_step_directive, step_params] = consume_directive<float>(it, "step", sources, lengths);
			if (is_step_directive) {
				auto& [step] = step_params;
				if (!step) fprintf(stderr, "Error parsing shader directive #step. Usage: #step(step)");
				std::string uniform_name = get_next_uniform_name(it);

				steps[uniform_name] = *step;
			}

			if (consume_directive(it, "color", sources, lengths)) {
				std::string uniform_name = get_next_uniform_name(it);
				is_color[uniform_name] = true;
			}


			if (check_token(it, "#type")) {
				lengths.push_back(it - sources[sources.size() - 1]);

				printf("End of shader source\n");
				break;
			}

			else if (check_token(it, "#include")) {
				// TODO: handle the include here
				lengths.push_back(it - sources[sources.size() - 1]);
				skip_to_newline(it);
				skip_whitespace(it);

				sources.push_back(it);
			}
		}

		else {
			skip_to_newline(it);
			skip_whitespace(it);
		}
	}

	if (!*it) {
		lengths.push_back(it - sources[sources.size() - 1]);

		printf("End of shader source\n");
	}

	uint32_t shader = glCreateShader(type);

	glShaderSource(shader, sources.size(), sources.data(), lengths.data());

	glCompileShader(shader);

	GLint _compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &_compiled);

	if (!_compiled) {
		std::string info_log;

		int32_t log_length = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
		info_log.resize(log_length);
		glGetShaderInfoLog(shader, log_length, &log_length, &info_log[0]);
	
		// TODO: Get shader type based on type variable
		fprintf(stderr, "Error in compilation of shader: \"\"\n"
			"\tIn {TODO} shader line %s\n", line_offset);
		std::cerr << "Failed to compile shader:\n" << info_log;

		glDeleteShader(shader);

		return -1;
	}


	return shader;
}



void Shader::load_from_string(const char* src) {
	// Use old fashioned C methods:
	// In my experience it's faster and it's not so hard to write
	// This might be premature optimization, but I feel like writing it this way today 😂

	// TODO: Make tokens non-case-sensitive
	// TODO: Deal with uniform related directives
	linked = false;

	uint32_t fragment_shader = 0;
	uint32_t vertex_shader = 0;
	uint32_t compute_shader = 0;
	uint32_t geometry_shader = 0;
	uint32_t tess_con_shader = 0;
	uint32_t tess_eval_shader = 0;

	int line = 0;

	gl_id = glCreateProgram();

	const char* it = src;
	skip_whitespace(it);

	// Go to the end of the file
	while (*it) {
		// A directive we care about
		if (*it == '#') {
			if (consume_token(it, "#type")) {
				skip_whitespace(it);

				if (consume_token(it, "fragment") || consume_token(it, "pixel")) {
					uint32_t shader = load_shader_src(GL_FRAGMENT_SHADER, it, line);
					if (shader != -1) {
						fragment_shader = shader;
						glAttachShader(gl_id, shader);
					}
				}
				
				else if (consume_token(it, "vertex")) {
					uint32_t shader = load_shader_src(GL_VERTEX_SHADER, it, line);
					if (shader != -1) {
						vertex_shader = shader;
						glAttachShader(gl_id, shader);
					}
				}
				
				else if (consume_token(it, "compute")) {
					uint32_t shader = load_shader_src(GL_COMPUTE_SHADER, it, line);
					if (shader != -1) {
						compute_shader = shader;
						glAttachShader(gl_id, shader);
					}
				}

				else if (consume_token(it, "geometry")) {
					uint32_t shader = load_shader_src(GL_GEOMETRY_SHADER, it, line);
					if (shader != -1) {
						geometry_shader = shader;
						glAttachShader(gl_id, shader);
					}
				}

				else if (consume_token(it, "tess_con")) {
					uint32_t shader = load_shader_src(GL_TESS_CONTROL_SHADER, it, line);
					if (shader != -1) {
						tess_con_shader = shader;
						glAttachShader(gl_id, shader);
					}
				}

				else if (consume_token(it, "tess_eval")) {
					uint32_t shader = load_shader_src(GL_TESS_EVALUATION_SHADER, it, line);
					if (shader != -1) {
						tess_eval_shader = shader;
						glAttachShader(gl_id, shader);
					}
				}

				else {
					skip_to_newline(it);
					skip_whitespace(it);
				}

			}
			else {
				skip_to_newline(it);
				skip_whitespace(it);
			}
		}
		else {
			skip_to_newline(it);
			skip_whitespace(it);
		}
	}

	// Now link the program
	glLinkProgram(gl_id);

	if (fragment_shader)	{ glDetachShader(gl_id, fragment_shader);	glDeleteShader(fragment_shader);  }
	if (vertex_shader)		{ glDetachShader(gl_id, vertex_shader);		glDeleteShader(vertex_shader);    }
	if (compute_shader)		{ glDetachShader(gl_id, compute_shader);	glDeleteShader(compute_shader);   }
	if (geometry_shader)	{ glDetachShader(gl_id, geometry_shader);	glDeleteShader(geometry_shader);  }
	if (tess_con_shader)	{ glDetachShader(gl_id, tess_con_shader);	glDeleteShader(tess_con_shader);  }
	if (tess_eval_shader)	{ glDetachShader(gl_id, tess_eval_shader);	glDeleteShader(tess_eval_shader); }

	GLint _linked;
	glGetProgramiv(gl_id, GL_LINK_STATUS, &_linked);

	if (_linked) linked = true;

	if (!linked) {
		int32_t log_length = 0;
		glGetProgramiv(gl_id, GL_INFO_LOG_LENGTH, &log_length);
		info_log.resize(log_length);
		glGetProgramInfoLog(gl_id, log_length, &log_length, info_log.data());// &info_log[0]);

		printf("%s\n", info_log.data());
		return;
	}

	// finally, introspect the shader!
	introspect();

}

void Shader::introspect() {
	// TODO: 
	//  * Support all types								(DONE!)
	//  * Persist across reloads						(DONE!)
	//  * Don't auto-persist unless value changed!!		(DONE! I hope)

	std::map<std::string, Uniform> old_uniforms; 
	old_uniforms.swap(uniforms);
	uniforms.clear();
	
	int num_active_uniforms = 0;
	glGetProgramiv(gl_id, GL_ACTIVE_UNIFORMS, &num_active_uniforms);

	for (int i = 0; i < num_active_uniforms; i++) {
		char uniform_name[1024];
		int length;
		uint32_t uniform_type;
		int uniform_size;

		glGetActiveUniform(gl_id, i, 1024, &length, &uniform_size, &uniform_type, uniform_name);
		ShaderDataType t = GetShaderDataType(uniform_type);

		if (is_color.contains(uniform_name)) {
			t = is_color[uniform_name] ? ShaderDataType::Color : t;
		}

		Uniform u(t);

		u.location = glGetUniformLocation(gl_id, uniform_name);
		u.name = uniform_name;


		switch (t) {
#define DATATYPE(type,func) case ShaderDataType::type:func(gl_id, u.location, (GetCPrimitiveType(ShaderDataType::type)*)&u.get_default<GetCType(ShaderDataType::type)>());break;
			DATATYPE(F32, glGetUniformfv);
			DATATYPE(F64, glGetUniformdv);

			DATATYPE(Vec2, glGetUniformfv);
			DATATYPE(Vec3, glGetUniformfv);
			DATATYPE(Vec4, glGetUniformfv);

			DATATYPE(Mat2, glGetUniformfv);
			DATATYPE(Mat3, glGetUniformfv);
			DATATYPE(Mat4, glGetUniformfv);

			DATATYPE(I32, glGetUniformiv);
			DATATYPE(U32, glGetUniformuiv);

			DATATYPE(Bool, glGetUniformiv);

			DATATYPE(Color, glGetUniformfv);
#undef DATATYPE

		default:
			fprintf(stderr, "Uniform type not supported for introspection: %s!\n", GetDataTypeName(t));

		}


		if (old_uniforms.contains(uniform_name) && old_uniforms[uniform_name].type == t && old_uniforms[uniform_name].changed) {
			switch (t) {
				#define X(type) case type: u.set(old_uniforms[uniform_name].get<GetCType(type)>()); break;
				FOREACH_SHADER_DATA_TYPE(X);
				#undef X
			default:
				assert("Type not implemented");
			}
		}
		else {
			u._data = u._default;
		}



		uniforms[u.name] = u;

	}
}

void Shader::use() {

	if (should_reload) {
		ranges.clear();
		steps.clear();
		is_color.clear();

		glDeleteProgram(gl_id);
		gl_id = 0;
		load_from_file(path.c_str());
		should_reload = false;
	}



	glUseProgram(gl_id);
	GL_ERROR_CHECK();

	// This should probably not be done this way, but it's how we're doing it for now 😂
	for (auto& [name, uniform] : uniforms) {
		switch (uniform.type) {
			// Let's do another macro thing here to save a lot of chars
#define UNIFORM_TYPE(shader_type, gl_func) case ShaderDataType::shader_type: gl_func(uniform.location, 1, (GetCPrimitiveType(ShaderDataType::shader_type)*)&uniform.get<GetCType(ShaderDataType::shader_type)>()); break;
#define MAT_UNIFORM_TYPE(shader_type, gl_func) case ShaderDataType::shader_type: gl_func(uniform.location, 1, false, (GetCPrimitiveType(ShaderDataType::shader_type)*)&uniform.get<GetCType(ShaderDataType::shader_type)>()); break;

			UNIFORM_TYPE(F32, glUniform1fv);
			UNIFORM_TYPE(F64, glUniform1dv);
			UNIFORM_TYPE(Vec2, glUniform2fv);
			UNIFORM_TYPE(Vec3, glUniform3fv);


			UNIFORM_TYPE(Vec4, glUniform4fv);

			MAT_UNIFORM_TYPE(Mat2, glUniformMatrix2fv);
			MAT_UNIFORM_TYPE(Mat3, glUniformMatrix3fv);
			MAT_UNIFORM_TYPE(Mat4, glUniformMatrix4fv);

			UNIFORM_TYPE(Color, glUniform3fv);

			UNIFORM_TYPE(I32, glUniform1iv);
			UNIFORM_TYPE(U32, glUniform1uiv);

			UNIFORM_TYPE(Bool, glUniform1iv);
#undef UNIFORM_TYPE
#undef MAT_UNIFORM_TYPE


		default:
			assert("UNKNOWN DATA TYPE");

		}

		auto error = glGetError();
		
		if (error != GL_NO_ERROR) {
			fprintf(stderr, "GL ERROR %d on Uniform %s\n", error, name.c_str());
		}
		
		GL_ERROR_CHECK();

	}

}

void Shader::show_imgui() {
	if (imgui_window_open) {

		if (ImGui::Begin("Shader Program", &imgui_window_open)) {

			// Let's render a control for each uniform!
			for (auto& [name, uniform] : uniforms) {
				ImGui::Tag uniform_id(&uniform);

				float min = -INFINITY;
				float max = INFINITY;

				bool has_range = false;

				if (ranges.contains(name)) {
					has_range = true;
					min = ranges[name].first;
					max = ranges[name].second;
				}

				float step = 0.1f;

				if (uniform.type == ShaderDataType::I32 || uniform.type == ShaderDataType::U32) step = 1.0f;

				if (steps.contains(name)) {
					step = steps[name];
				}

				switch (uniform.type) {

#define RenderControl(DataType, ImGuiFunc, ...) case ShaderDataType::DataType: if(ImGui::ImGuiFunc(name.c_str(), (GetCPrimitiveType(ShaderDataType::DataType)*)&uniform.get<GetCType(ShaderDataType::DataType)>(), __VA_ARGS__)) uniform.changed = true; break;										
					RenderControl(F32, DragFloat, step, min, max);
					RenderControl(Vec2, DragFloat2, step, min, max);
					RenderControl(Vec3, DragFloat3, step, min, max);
					RenderControl(Vec4, DragFloat4, step, min, max);
					RenderControl(I32, DragInt, (int)step);
					RenderControl(Color, ColorEdit3);
#undef RenderControl

				case ShaderDataType::Bool:
					if (ImGui::Checkbox(name.c_str(), (bool*)&uniform.get<uint32_t>())) {
						uniform.changed = true; 
					}
					break;
					//case ShaderDataType::Vec3: if(ImGui::DragFloat3(name.c_str(), (float*)&uniform.get<glm::vec3>(), 0.1f)) uniform.changed = true; break;

				default: continue;
				}

				ImGui::SameLine();
				if (ImGui::Button("Reset")) {
					uniform._data.m4 = uniform._default.m4;
					uniform.changed = false;
				}
				

			}

			ImGui::End();
		}
	}
}

void Shader::unload() {
	uniforms.clear();
	glDeleteProgram(gl_id);
	gl_id = 0;
}