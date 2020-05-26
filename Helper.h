#pragma once

template<typename out, class type>
inline out method(size_t index, type* self)
{
    return reinterpret_cast<out>((*reinterpret_cast<void***>(self))[index]);
}