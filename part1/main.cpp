#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
using namespace std::chrono_literals;

#include <cpprest/http_msg.h>
#include <wiringPi.h>

#include "metronome.hpp"
#include "rest.hpp"


#define LED_GREEN 21
#define LED_RED   26
#define BTN_MODE  20
#define BTN_TAP   16

metronome m;
std::atomic<bool> learn_mode(false);
std::atomic<bool> mode_button_pressed(false);
std::atomic<bool> tap_button_prev_state(false);

std::mutex bpm_lock;
size_t bpm = 0, bpm_min = SIZE_MAX, bpm_max = 0;

void blink_loop() {
	while (true) {
		if (!learn_mode && bpm > 0) {
			digitalWrite(LED_GREEN, HIGH);
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			digitalWrite(LED_GREEN, LOW);
			std::this_thread::sleep_for(std::chrono::milliseconds(60000 / bpm - 100));
		} else {
			digitalWrite(LED_GREEN, LOW);
			std::this_thread::sleep_for(100ms);
		}
	}
}

void button_poll_loop() {
	bool prev_mode = false;
	bool prev_tap = false;

	while (true) {
		bool mode_now = digitalRead(BTN_MODE) == HIGH;
		bool tap_now  = digitalRead(BTN_TAP) == HIGH;

		// Detect mode button rising edge
		if (mode_now && !prev_mode) {
			learn_mode = !learn_mode;

			if (learn_mode) {
				m.start_timing();
			} else {
				m.stop_timing();
				size_t new_bpm = m.get_bpm();

				std::lock_guard<std::mutex> guard(bpm_lock);
				bpm = new_bpm;
				if (bpm != 0) {
					if (bpm < bpm_min) bpm_min = bpm;
					if (bpm > bpm_max) bpm_max = bpm;
				}
			}
		}

		// In learn mode, record taps on rising edge
		if (learn_mode && tap_now && !prev_tap) {
			digitalWrite(LED_RED, HIGH);
			m.tap();
			std::this_thread::sleep_for(50ms);
			digitalWrite(LED_RED, LOW);
		}

		prev_mode = mode_now;
		prev_tap  = tap_now;

		std::this_thread::sleep_for(10ms);
	}
}

// === REST ENDPOINT HANDLERS ===
void get_bpm(web::http::http_request req) {
	std::lock_guard<std::mutex> guard(bpm_lock);
	req.reply(200, web::json::value::number(bpm));
}

void put_bpm(web::http::http_request req) {
	req.extract_json().then([req](web::json::value body) {
		if (body.is_number()) {
			size_t new_bpm = body.as_integer();
			std::lock_guard<std::mutex> guard(bpm_lock);
			bpm = new_bpm;

			//Force system into PLAY mode
			learn_mode = false;

			if (bpm < bpm_min) bpm_min = bpm;
			if (bpm > bpm_max) bpm_max = bpm;

			req.reply(200, "BPM updated.");
		} else {
			req.reply(400, "Invalid JSON.");
		}
	});
}

void get_bpm_min(web::http::http_request req) {
	std::lock_guard<std::mutex> guard(bpm_lock);
	req.reply(200, web::json::value::number((bpm_min == SIZE_MAX) ? 0 : bpm_min));
}

void del_bpm_min(web::http::http_request req) {
	std::lock_guard<std::mutex> guard(bpm_lock);
	bpm_min = SIZE_MAX;
	req.reply(200, "Min BPM reset.");
}

void get_bpm_max(web::http::http_request req) {
	std::lock_guard<std::mutex> guard(bpm_lock);
	req.reply(200, web::json::value::number(bpm_max));
}

void del_bpm_max(web::http::http_request req) {
	std::lock_guard<std::mutex> guard(bpm_lock);
	bpm_max = 0;
	req.reply(200, "Max BPM reset.");
}

int main() {
	wiringPiSetupGpio();

	pinMode(LED_GREEN, OUTPUT);
	pinMode(LED_RED, OUTPUT);
	pinMode(BTN_MODE, INPUT);
	pinMode(BTN_TAP, INPUT);
	pullUpDnControl(BTN_MODE, PUD_DOWN);
	pullUpDnControl(BTN_TAP, PUD_DOWN);

	// REST endpoints
	auto bpm_ep = rest::make_endpoint("/bpm/");
	bpm_ep.support(web::http::methods::GET, get_bpm);
	bpm_ep.support(web::http::methods::PUT, put_bpm);

	auto min_ep = rest::make_endpoint("/bpm/min/");
	min_ep.support(web::http::methods::GET, get_bpm_min);
	min_ep.support(web::http::methods::DEL, del_bpm_min);

	auto max_ep = rest::make_endpoint("/bpm/max/");
	max_ep.support(web::http::methods::GET, get_bpm_max);
	max_ep.support(web::http::methods::DEL, del_bpm_max);

	bpm_ep.open().wait();
	min_ep.open().wait();
	max_ep.open().wait();

	std::thread(blink_loop).detach();
	std::thread(button_poll_loop).detach();

	while (true)
		std::this_thread::sleep_for(1s);

	return 0;
}
