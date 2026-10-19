#include "alpha.preface.h"
#include "alpha.typeid.h"
#include "alpha.int.h"
#include "alpha.bool.h"

/*

	Nod source:

	type thing;

	type widget from ( thing, bool )
	{
		general
		{
			begin
			{
			}
		}

		abstract
		{
			squiggle
			{
			}
		}

		misc
		{
			end
			{
			}
		}


		instance
		{
			int x;
			int y;
		}
	}


*/

extern type thing_type;

struct thing_object : public any_object
{
	thing_object()
	{
		this->any_object::set_type( &thing_type );
	}
};



extern type widget_type;


struct widget_object : public any_object
{
	thing_object	base_thing;  // base_ + Nod base type name
	bool_object		base_bool;

	int_object		x;		// Nod instance name
	int_object		y;

	widget_object()
	{
		this->any_object::set_type( &widget_type );
	}

	// Nod procedure functions...

	static void begin ( oref<widget_object> )  
	{
	}

	static void end ( oref<widget_object> )
	{
	}

	static void zz7Eadapt ( iref<widget_object> )  // Nod name = ~adapt
	{
	}
};



PLATFORM_INT4_TYPE widget_offsets [] =
{
	( PLATFORM_INT4_TYPE )offsetof( widget_object, base_thing ),
	( PLATFORM_INT4_TYPE )offsetof( widget_object, base_bool ),
};

void* widget_adaptation [] =
{
	&widget_object::zz7Eadapt
};

type widget_type =
{
	"widget",
	"MyBookByMe.Source.widget",
	2,
	widget_offsets,
	&widget_object::end,
	widget_adaptation
};



/*
	Nod source...

	routine foo 
	{
		int x;
		widget w;
	}
*/


void foo ()
{
	int_object x;  
	widget_object w;
}

