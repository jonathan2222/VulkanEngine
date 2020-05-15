#include "stdafx.h"
#include "CircularBuffer.h"

void ym::CircularBuffer::init(uint64_t sampleRate)
{
	this->sampleRate = sampleRate;
	this->size = sampleRate * MAX_CIRCULAR_BUFFER_SIZE * 2;
	this->data = new float[this->size];
	memset(this->data, 0, this->size * sizeof(float));
}

void ym::CircularBuffer::clearAll()
{
	memset(this->data, 0, this->size * sizeof(float));
}

void ym::CircularBuffer::clear()
{
	memset(this->data, 0, (this->endIndex+1) * sizeof(float));
}

void ym::CircularBuffer::destroy()
{
	SAFE_DELETE_ARR(this->data);
}

void ym::CircularBuffer::setDelay(double seconds)
{
	this->endIndex = glm::min((int64_t)(this->sampleRate * seconds * 2), (int64_t)this->size - 1);
}

void ym::CircularBuffer::setDelayInSamples(int64_t samples)
{
	this->endIndex = glm::min(samples * 2 - 1, (int64_t)this->size - 1);
}

float ym::CircularBuffer::get() const
{
	return this->data[this->end];
}

void ym::CircularBuffer::add(float v)
{
	this->data[this->start] = v;
	next();
}

void ym::CircularBuffer::next()
{
	auto nextV = [&](uint64_t v)->uint64_t {
		return (++v) % (this->endIndex + 1);
	};

	this->start = nextV(this->start);
	this->end = nextV(this->end);
}
