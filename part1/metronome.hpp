#pragma once
#include <cstddef>

class metronome {
public:
	static const int beat_samples = 3;

	metronome();
	void start_timing();
	void stop_timing();
	void tap();
	size_t get_bpm() const;
	bool is_timing() const { return m_timing; }

private:
	bool m_timing;
	size_t m_beats[beat_samples];
	size_t m_beat_count;
};
