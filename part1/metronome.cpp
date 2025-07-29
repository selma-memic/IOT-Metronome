#include "metronome.hpp"
#include <chrono>

using namespace std::chrono;

static steady_clock::time_point last_time;

metronome::metronome() : m_timing(false), m_beat_count(0) {}

void metronome::start_timing() {
	m_timing = true;
	m_beat_count = 0;
	last_time = steady_clock::now();
}

void metronome::stop_timing() {
	m_timing = false;
}

void metronome::tap() {
	if (!m_timing) return;

	auto now = steady_clock::now();
	auto delta = duration_cast<milliseconds>(now - last_time).count();
	last_time = now;

	if (m_beat_count < beat_samples) {
		m_beats[m_beat_count++] = delta;
	} else {
		for (size_t i = 1; i < beat_samples; ++i)
			m_beats[i - 1] = m_beats[i];
		m_beats[beat_samples - 1] = delta;
	}
}

size_t metronome::get_bpm() const {
	if (m_beat_count < 2) return 0;

	size_t sum = 0;
	for (size_t i = 0; i < m_beat_count; ++i)
		sum += m_beats[i];

	size_t avg = sum / m_beat_count;
	if (avg == 0) return 0;

	return 60000 / avg;  // convert ms to BPM
}
