#pragma once

#include <chrono>
#include <string>
#include <map>
#include <vector>
#include <array>


#include <imgui.h>

class Instrumentor {
public:
	// Instrumentor will be a singleton type class for ease of use.
	using Clock = std::chrono::high_resolution_clock;
	using TimeStamp = std::chrono::time_point<Clock>;
	using Duration = std::chrono::duration<double, std::nano>;
	struct InstrumentationEntry;


	static Instrumentor& get() {
		static Instrumentor instrumentor;
		return instrumentor;
	}


	void new_frame() {
		// Traverse the tree of entries, and set all times to zero.
		// This is to accomodate functions that are only sometimes called

		if (m_current_entry) {
			end_frame();
		}

		iterate_tree([](const std::string& name, Ref<InstrumentationEntry> entry, int level) {
			entry->frames_alive = std::min(entry->frames_alive + 1, 1024ull);
			entry->mean_time = entry->sum_time / entry->frames_alive;

			entry->circular_buffer_idx = (entry->circular_buffer_idx + 1) % 1024;
			entry->sum_time -= entry->durations[entry->circular_buffer_idx];
			entry->durations[entry->circular_buffer_idx] = 0.0;
		});

		m_frame->last_start_time = get_time();
		m_current_entry = m_frame;
	}

	void end_frame() {
		assert(m_current_entry == m_frame);
		end_event();
	}


	void show_profiler() {
		if (ImGui::Begin("Instrumetor")) {
			iterate_tree([](const std::string& name, Ref<InstrumentationEntry> entry, int level) {
				if (entry) {
					ImGui::Indent(level);
					ImGui::LabelText(name.c_str(), "%f ms", entry->mean_time / 1000.0 / 1000.0);
				}
			});
			
		}
		ImGui::End();
	}

	struct InstrumentationEvent {
		InstrumentationEvent(std::string name) {
			Instrumentor& instrumentor = Instrumentor::get();
			instrumentor.start_event(name);
		}

		~InstrumentationEvent() {
			Instrumentor& instrumentor = Instrumentor::get();
			instrumentor.end_event();
		}
	};



protected:
	TimeStamp get_time() {
		return m_clock.now();
	}


	void iterate_tree(std::function<void(const std::string&, Ref<InstrumentationEntry>, int)> callback, const std::string& name, Ref<InstrumentationEntry> entry, int level) {
		callback(name, entry, level);

		for (auto& [name, child] : entry->children) {
			iterate_tree(callback, name, child, level+1);
		}
	}

	void iterate_tree(std::function<void(const std::string&, Ref<InstrumentationEntry>, int)> callback) {
		iterate_tree(callback, "Frame", m_frame, 0);
	}




	// Adds an event onto the event tree
	void start_event(std::string name) {
		// Make the child entry if we need to
		if (!m_current_entry->children.contains(name)) {
			m_current_entry->children[name] = make_ref<InstrumentationEntry>();
			m_current_entry->children[name]->parent = make_weak_ref(m_current_entry);
		}

		// First set the current entry to the child we are looking for
		m_current_entry = m_current_entry->children[name];
		
		// Then start the timer
		m_current_entry->last_start_time = get_time();
	}

	// Ends the last started event
	void end_event() {
		double duration = (get_time() - m_current_entry->last_start_time).count();
		m_current_entry->durations[m_current_entry->circular_buffer_idx] += duration; // += so multiple calls will accumulate to a single time
		m_current_entry->sum_time += duration;										  // note that calls from different parent functions will not accumulate this way
		m_current_entry->mean_time = m_current_entry->sum_time / 1024.0;
		m_current_entry = m_current_entry->parent.lock();
	}



private:
	Instrumentor() : m_clock() {
		start_time = m_clock.now();
		m_frame = make_ref<InstrumentationEntry>();
	}

	struct InstrumentationEntry {
		WeakRef<InstrumentationEntry> parent = {};
		std::map<std::string, Ref<InstrumentationEntry>> children = {};

		std::array<double, 1024> durations = {}; // we will store the last 1024 durations
		double mean_time = 0.0; // the mean time for this event
		double sum_time = 0.0;
		size_t circular_buffer_idx = 0;

		TimeStamp last_start_time = {};
		size_t frames_alive = 0;
	};



	Ref<InstrumentationEntry> m_frame;
	Ref<InstrumentationEntry> m_current_entry;

	std::map <std::string, InstrumentationEntry> m_entries;

	Clock m_clock;
	
	TimeStamp start_time;
};


#define PROFILE2(name, line) Instrumentor::InstrumentationEvent _profile_##line(name);

#define PROFILE(name, line) PROFILE2(name, line);
#define PROFILE_SCOPE(name) PROFILE(name, __LINE__);
#define PROFILE_FUNC() PROFILE_SCOPE(__FUNCTION__);
