#pragma once
#include <array>

template <typename T, int X, int Y>
class array2D {
	public:
		std::array<T, X*Y> data;

		array2D() { clear(); }

		constexpr int width()  const { return X; };
		constexpr int height() const { return Y; };

		bool valid(int x, int y) const {
			return x >= 0 && y >= 0 && x < X && y < Y;
		}

		size_t index(int x, int y) const {
			return y*X + x;
		}

		T get(int x, int y) const {
			if (valid(x, y)) {
				return data[index(x, y)];
			}

			return T();
		}

		void set(int x, int y, T value) {
			if (valid(x, y)) {
				data[index(x, y)] = value;
			}
		}

		void clear(void) {
			for (int x = 0; x < X; x++) {
				for (int y = 0; y < Y; y++) {
					set(x, y, 0);
				}
			}
		}

		void floodfill(int x, int y, T target, T replace) {
			if (!valid(x, y)) return;

			if (get(x, y) == target) {
				set(x, y, replace);
				//cur = replace;

				floodfill(x + 1, y, target, replace);
				floodfill(x - 1, y, target, replace);
				floodfill(x, y + 1, target, replace);
				floodfill(x, y - 1, target, replace);
			}
		}

		void clearExcept(T target, T replace) {
			for (int x = 0; x < X; x++) {
				for (int y = 0; y < Y; y++) {
					if (get(x, y) != target) {
						set(x, y, replace);
					}
				}
			}
		}
};
