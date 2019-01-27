#ifndef __GNUC__
#pragma once
#endif
#ifndef _KD_VECTOR_H_
#define _KD_VECTOR_H_
#include "../../xray_re/xr_types.h"
#if 0
template <typename T> class dynamic_array {
public:
	dynamic_array();
	dynamic_array(size_t n);
	dynamic_array(size_t n, T& val);
	~dynamic_array();

	T&			at(size_t pos);
	const T&	at(size_t pos) const;
	T&			operator[](size_t pos);
	const T&	operator[](size_t pos) const;

	void		push_back(T val);
	void		resize(size_t size);
	void		reserve(size_t size);
//	void		remove(T val);
	void		clear();
	bool		empty();
	T*&			data();
	size_t		size();
	size_t		capacity();

	class const_iterator {
	public:
		const_iterator() {value = NULL};
		const_iterator(T& _value):value(&_value) {};
		const_iterator(T*& _value):value(_value) {};
		const T*&					operator->() {return value;};
		const T&					operator*() {return *value;};
		const const_iterator&		operator++() {++value; return *this;}
		const const_iterator&		operator--() {--value; return *this;}
		const const_iterator&		operator++(int) {return iterator(value++);}
		const const_iterator&		operator--(int) {return iterator(value--);}
		bool						operator==(const const_iterator& right) {return value == right.value;}
		bool						operator!=(const const_iterator& right) {return value != right.value;}
		bool						operator<(const const_iterator& right) {return value < right.value;}
	protected:
		T*	value;
	};

	class iterator:public const_iterator {
	public:
		iterator() {value = NULL};
		iterator(T& _value):value(&_value) {};
		iterator(T*& _value):value(_value) {};
		T*&			operator->() {return value;};
		T&			operator*() {return *value;};
		iterator&	operator++() {++value; return *this;}
		iterator&	operator--() {--value; return *this;}
		iterator&	operator++(int) {return iterator(value++);}
		iterator&	operator--(int) {return iterator(value--);}
		bool		operator==(const iterator& right) {return value == right.value;}
		bool		operator!=(const iterator& right) {return value != right.value;}
		bool		operator<(const iterator& right) {return value < right.value;}
	};

	iterator			begin() {return _first;};
	iterator			end() {return _last;};
	const_iterator		begin() const {return _first;};
	const_iterator		end() const {return _last;};

private:
	iterator		_first;
	iterator		_last;
	iterator		_end;
};

template <typename T> inline dynamic_array<T>::dynamic_array() {};
template <typename T> inline dynamic_array<T>::dynamic_array(size_t n) 
{
	T* arr = new T[n];
	_first = arr;
	_last = arr + n;
	_end = _last;
};
template <typename T> inline dynamic_array<T>::dynamic_array(size_t n, T& val) 
{
	T* arr = new T[n];
	_first = arr;
	_last = arr + n;
	_end = _last;
	for (size_t i = 0; i < n; ++i)
		_first[i] = val;
};
template <typename T> inline dynamic_array<T>::~dynamic_array() {delete[] &(*_first);};
template <typename T> inline T& dynamic_array<T>::at(size_t pos) 
{
	xr_assert(!empty());
	xr_assert(pos < size());
	return _first->[pos];
};
template <typename T> inline const T& dynamic_array<T>::at(size_t pos) const
{
	xr_assert(!empty());
	xr_assert(pos < size());
	return _first->[pos];
};
template <typename T> inline T& dynamic_array<T>::operator[](size_t pos)
{
	xr_assert(!empty());
	xr_assert(pos < size());
	return _first->[pos];
};
template <typename T> inline const T& dynamic_array<T>::operator[](size_t pos) const
{
	xr_assert(!empty());
	xr_assert(pos < size());
	return _first->[pos];
};
template <typename T> inline bool dynamic_array<T>::empty() {return _first == _last;}
template <typename T> inline size_t dynamic_array<T>::size() {return (&(*_last) - &(*_first)) / sizeof(T*);}
template <typename T> inline size_t dynamic_array<T>::capacity() {return (&(*_end) - &(*_first)) / sizeof(T*);}
template <typename T> inline T*& dynamic_array<T>::data() 
{
	xr_assert(!empty());
	return &(*_first);
}
template <typename T> inline void dynamic_array<T>::clear() 
{
	if (_last != _first)
		_last = _first;
}
/*
template <typename T> inline void dynamic_array<T>::remove(T val)
{
	xr_assert(_size > 0);
	size_t pos = 0;
	for (size_t i = 0; i < _size, ++i) {
		if (_first[i] == val) {
			pos = i;
			break;
		}
	}
	if (pos < _size) {
		for (size_t i = pos; i < (_size - 1); ++i)
			_first[i] = _first[i+1];
		--_size;
	}
}
*/
template <typename T> inline void dynamic_array<T>::reserve(size_t n)
{
	xr_assert(n > 0);
	if (n > size()) {
		T* arr = new T[n];
		if (size() != 0) {
			for (size_t i = 0; i < size(); ++i)
				*arr++ = *_first++;
		}
		_first = arr;
		_last = _first;
		_end = iterator(arr + n);
	}
}
template <typename T> inline void dynamic_array<T>::resize(size_t n)
{
	xr_assert(n > 0);
	if (n > size()) {
		if (n > capacity())
			reserve(n);
	}
	_last = _first + n;
}
template <typename T> inline void dynamic_array<T>::push_back(T val)
{
	xr_assert(size() > 0);
	if (_last == _end)
		resize(size() * 1.5);
	_last = val;
	++_last;
};
#endif
template <typename T> class static_array {
public:
	static_array();
	static_array(size_t n);
	~static_array();

	void		reserve(size_t n);
	void		push_back(T& val);
	void		insert(size_t pos, T& val);
	void		insert(size_t pos, size_t num, T*& val);

	void		remove_fast(size_t pos, bool destruct = false);
	void		remove_keep_sorting(size_t pos, bool destruct = false);

	size_t		size();
	size_t		capacity();
	bool		empty();

	T&			operator[](size_t pos);
	const T&	operator[](size_t pos) const;

private:
	size_t	m_size;
	size_t	m_capacity;
	T*		m_values;
};
template <typename T> inline static_array<T>::static_array():m_values(NULL), m_size(0), m_capacity(0) {};
template <typename T> inline static_array<T>::static_array(size_t n) {reserve(n);};
template <typename T> inline static_array<T>::~static_array()
{
	if (m_values)
		delete[] m_values;
};
template <typename T> inline void static_array<T>::reserve(size_t n)
{
	if (n > 0)
		m_values = new T[n];
	m_size = 0;
	m_capacity = n;
};
template <typename T> inline T& static_array<T>::operator[](size_t pos)
{
	xr_assert(pos < m_size);		// is within range
	return m_values[pos];
}
template <typename T> inline const T& static_array<T>::operator[](size_t pos) const
{
	xr_assert(pos < m_size);		// is within range
	return m_values[pos];
}
template <typename T> inline size_t static_array<T>::size() {return m_size};
template <typename T> inline size_t static_array<T>::capacity() {return m_capacity};
template <typename T> inline bool static_array<T>::empty() {return m_size == 0};
template <typename T> inline void static_array<T>::push_back(T& val)
{
	xr_assert(m_values);				// is initialized
	xr_assert(m_size < m_capacity);		// has empty place
	if (val) {							// is not zero arg
		m_values[m_size++] = val;
	}
};
template <typename T> inline void static_array<T>::insert(size_t pos, T& val)
{
	xr_assert(m_values);				// is initialized
	xr_assert(pos < m_size);			// is valid position
	xr_assert(m_size < m_capacity);		// has empty place
	if (val) {							// is not zero arg
		for (size_t i = m_size; i > pos; --i)
			m_values[i + 1] = m_values[i];
		m_values[pos] = val;
		++m_size;
	}
};
template <typename T> inline void static_array<T>::insert(size_t pos, size_t num, T*& val)
{
	xr_assert(m_values);				// is initialized
	xr_assert(pos < m_size);			// is valid position
	xr_assert((m_size + num) < m_capacity);		// has empty place
	xr_assert(num > 0);					// is valid num of inserted elements
	if (val) {							// is not zero arg
		for (size_t i = m_size; i > pos; --i)
			m_values[i + num] = m_values[i];
		for (size_t i = pos; i < pos + num; ++i)
			m_values[i] = *val++;
		m_size += num;
	}
};
template <typename T> inline void static_array<T>::delete_fast(size_t pos, bool destruct)
{
	xr_assert(m_values);				// is initialized
	xr_assert(pos < m_size);			// is valid position
	if (destruct)
		m_values[pos].~T();
	m_values[pos] = m_values[m_size--];
}
template <typename T> inline void static_array<T>::delete_keep_sorting(size_t pos, bool destruct)
{
	xr_assert(m_values);				// is initialized
	xr_assert(pos < m_size);			// is valid position
	if (destruct)
		m_values[pos].~T();
	--m_size;
	for (size_t i = pos; i < m_size; ++i)
		m_values[i] = m_values[i + 1];
}
#endif