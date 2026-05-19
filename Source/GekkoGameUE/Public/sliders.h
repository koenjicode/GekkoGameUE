#pragma once

#include <stdio.h>
#include <stdlib.h>


namespace SliderGame
{
	constexpr int MAX_PLAYERS = 4;
	constexpr int GAME_SCALE = 1000;
	constexpr int MAP_SIZE = 400;
	
	constexpr uint8_t PLAYER_COLORS[4 * MAX_PLAYERS] = {
		255, 0, 0, 255,
		0, 255, 0, 255,
		71, 167, 199, 255,
		255, 0, 255, 255,
	};

	struct Input {
		uint8_t left : 1;
		uint8_t right : 1;
		uint16_t padd = 0;
	};

	struct State {
		int entt_px[MAX_PLAYERS];

		void tick(Input inputs[MAX_PLAYERS]) {
			// move sliders
			for (int i = 0; i < MAX_PLAYERS; i++) {
				if (inputs[i].left) entt_px[i] -= 6000;
				else if (inputs[i].right) entt_px[i] += 6000;
			}

			// clamp to screen
			const int max = GAME_SCALE * (MAP_SIZE * 2 - MAP_SIZE / MAX_PLAYERS);
			for (int i = 0; i < MAX_PLAYERS; i++) {
				if (entt_px[i] > max) entt_px[i] = max;
				else if (entt_px[i] < 0) entt_px[i] = 0;
			}
		}
	};
}

