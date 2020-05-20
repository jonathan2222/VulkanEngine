#pragma once

#include <string>
#include <complex>
#include <glm/gtc/constants.hpp>

namespace ym
{
	class Utils
	{
	public:
		static std::string getNameFromPath(const std::string& path)
		{
			std::string name = path;
			const size_t last_slash_idx = name.find_last_of("\\/");
			if (std::string::npos != last_slash_idx)
				name.erase(0, last_slash_idx + 1);
			return name;
		}

		static void fft(float* inX, std::complex<float>* outX, uint32_t N, uint32_t s)
		{
			/*constexpr float pi = glm::pi<float>();
			std::complex<float> W(std::cos(2 * pi/N), -sin(2*pi/N));
			for (uint32_t k = 0; k < N; k++)
			{
				std::complex<float> sum = 0.f;
				for (uint32_t i = 0; i < N; i++)
				{
					std::complex<float> nW = std::pow(W, k*i);
					sum += nW * inX[i];
				}
				outX[k] = sum;
			}*/

			constexpr float pi = glm::pi<float>();
			if (N == 1)
				outX[0] = std::complex<float>(0.f);
			else
			{
				fft(inX, outX, N / 2, 2 * s);
				fft(inX + s, outX + s, N / 2, 2 * s);
				std::complex<float> W(std::cos(2 * pi / N), -sin(2 * pi / N));
				for (uint32_t k = 0; k < N / 2; k++)
				{
					std::complex<float> nW = std::pow(W, k);
					std::complex<float> c(outX[k]);
					outX[k] = c + nW*outX[k];
					outX[k + N/2] = c - nW * outX[k+N/2];
				}
			}
		}

		static std::vector<std::complex<float>> fft2(float* inX, uint32_t N, uint32_t s)
		{
			std::vector<std::complex<float>> arr;
			constexpr float pi = glm::pi<float>();
			if (N == 1)
			{
				arr.push_back(std::complex<float>(inX[0]));
			}
			else
			{
				auto arr2 = fft2(inX, N / 2, 2 * s);
				arr.insert(arr.begin(), arr2.begin(), arr2.end());
				auto arr3 = fft2(inX + s, N / 2, 2 * s);
				arr.insert(arr.begin(), arr3.begin(), arr3.end());
				std::complex<float> W(std::cos(-2 * pi / N), sin(-2 * pi / N));
				for (uint32_t k = 0; k < N / 2; k++)
				{
					std::complex<float> nW = std::pow(W, k);
					std::complex<float> c(arr[k]);
					arr[k] = c + nW*arr[k + N / 2];
					arr[k + N / 2] = c - nW*arr[k+N/2];
				}
			}
			return arr;
		}

		static void ifft(float* inX, float* outX, uint32_t N)
		{

		}
	};
}