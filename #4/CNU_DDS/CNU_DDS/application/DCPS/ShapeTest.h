#ifndef __SHAPE_TEST_H__
#define __SHAPE_TEST_H__

namespace CNU_DDS
{
	typedef struct ShaptType_t
	{
		long		lengthOfColor;	// NULL length ����
		char		color[128];	// 4������ ����
		long		x;
		long		y;
		long		shapesize;
	}ShapeType;

	void*	write_shapetype(void* arg);
	void	ShapeTest();
}

#endif