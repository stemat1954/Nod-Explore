#pragma once

/*
	This file is globally included. It defines the common
	elements of C++ code generation.

	Generated C code is technically C++. However, class 
	mechanisms are limited to very simple derivation, 
	templates, and member functions. 

	NOTE: How disruptions are handled is TBD but probably
	throw is the only option. Translation of conditional
	HNM instructions is	also TBD but not relevant to 
	implmentation of this library.

	Every Nod type has two corresponding items in C:

	(1) A type record that describes the type

	(2) A type-object struct that is the C type used to
	instantiate Nod objects in the generated code. The
	struct name is xxx_object where xxx is the Nod type name
	(made compatible - see below).
	
	Type records are configured per type in static C memory.

	Instantiating a default type-object in generated code 
	defines a null Nod object bound to the static type 
	record.  For example:

		int_object n; 
		
	is generated C code that corresponds to Nod "int n",
	which has a corresponding HNM MAKE instruction.


	Alpha types follow the same pattern but they're manually
	coded.  There is a source file pair for each Alpha type.
	Nod prototypes are in the NDK alpha book. Nod prototypes 
	and the C implementations in this library are tightly 
	coupled!

	Because Nod allows types to freely reference other types 
	in the same book, most type-object C functions need to be 
	written in the object source (cpp) file to avoid irrecon-
	cilable problems related to pre-inclusion. Forward type-
	object declarations are defined at the top of this file 
	to allow C function prototypes.

	NOTE:  This may be changed in the future to improve final
	code generation.


	Nod names have fewer restrictions than C, so transla-
	tion is generally required. Nod special characters are
	translated to ascii hex codes and a prefix is added if
	the Nod name starts with a number or special character.
	Generic factors are also replaced.

		!			->	21
		&			->	26
		?			->	3F
		@			->	40
		~			->	7E
		<foo>		-> _foo_
		<#foo>		-> _xfoo_
		(prefix)	->	zz 


	get_c_name() can be called by the compiler to translate
	a Nod name to a C name.


	Nod procedure functions use the following interface 
	and calling convention:
	
	(1) static function that returns void

	(2) method imo is given as oref<t>, iref<t>, or 
		irefu<t>, where <t> is the struct type, depending
		on primary intent

	(3) regular inputs are given as iref<t> where <t> is
		the input type

	(4) marked [upd] inputs are given as irefu<t> where <t>
		is the input type

	(5) outputs are given as t* where t is a the output type;
		the given object is default constructed (null)

	(6) extra collection is given as extra_object*. Compiler
		loads the collection

	(7) result proxies are given as proxy<t>* where t is 
		the proxy type; the given proxy is default construct-
		ed (void)

	(8) adapter routines are given as function prototype *

	(9) adhoc types are given as type*

	Everything given in Nod procedure order, except adhoc 
	type pointers are given first, before others.

	iref<t> and irefu<t> derive from oref<t> and have the
	same pointers, but each has a different constraint mech-
	anism. The difference allows C function	signatures to
	be matched to Nod procedure signatures and allows easier
	runtime constraint management.


	structs and functions are scoped to C namespaces in a 
	manner that corresponds to Nod entity domains:
	
	(1) All names are scoped to a namespace that corre-
	sponds to the Nod book name.
	
	(2) Names that are internal to a page are scoped to a
	page namespace.
	
	(3) Any static method function, or any name that is 
	internal to a Nod type is scoped to the corresponding
	type-object struct.

	(4) Any name that is internal to a Nod procedure is
	scoped to the corresponding C function.
 

	This library is designed to be linked with the Nod 
	compiler and generated C code. Compiler functionality 
	is generally provided by regular C member functions.
	These functions	provide various mechanisms for compile-
	time optimization and verification.  In particular, 
	many type-objects provide literal validation and pre-
	initialzation.
*/


// forward object declarations

struct any_object;
struct typeid_object;
struct bool_object;
struct addr_object;
struct expr_object;
struct int_object;
struct mediator_object;



struct type
{
	const char*			p_name;			// Nod name
	const char*			p_key;			// global key = domain qualified name = book.page...name 
	PLATFORM_INT2_TYPE	n_offsets;		// number of external sub-objects
	PLATFORM_INT4_TYPE*	p_offsets;		// external sub-object offsets, if any 
	void*				p_end;			// end method, if any
	void**				p_adapt;		// adaptation, if any
};



struct any_proxy
/*
	Joining an object and setting flags is done once and 
	only once in a proxy's lifetime.

	eval and upd constraint flags are mutually exclusive.

	stale and dead flags are redundant but Nod semantics
	allow it.

	Proxies spawned from a single object are linked in a 
	two-way chain:

	object ->		p1 -> p2 -> p3 -> 0
			   0 <-	p1 <- p2 <- p3

	
	Deconstruction unlinks this from the chain. If this prev
	pointer is null (p1 in the diagram), this proxy is first
	in the chain, so the object must be relinked to the new 
	first proxy (p2 in the diagram). 

	Relinking to the object MUST be done from the body of
	the typed proxy which is linked to the object, before
	the any-proxy destructor is called.
*/
{
	enum
	{
		eval_flag	= 0x01,
		upd_flag	= 0x02,
		stale_flag	= 0x04,
		dead_flag	= 0x08
	};

	any_proxy*			p_next;
	any_proxy*			p_prev;
	unsigned char		flags;   // eval, upd, stale, dead

	any_proxy ()
	{
		this->p_next = nullptr;
		this->p_prev = nullptr;
		this->flags = 0;
	}

	~any_proxy ()
	{
		if ( this->p_next != nullptr ) this->p_next->p_prev = this->p_prev;  // may be nullptr
	}

	void set_eval ()
	{
		this->flags &= any_proxy::eval_flag;
	}

	bool test_eval () const
	{
		return ( ( this->flags & any_proxy::eval_flag ) != 0 );
	}


	void set_upd ()
	{
		this->flags &= any_proxy::upd_flag;
	}

	bool test_upd () const
	{
		return ( ( this->flags & any_proxy::upd_flag ) != 0 );
	}

	
	int get_access () const
	{
		return ( this->test_eval() ? 0 : ( this->test_upd() ? 1 : 2 ) );
	}


	void set_stale ()
	{
		this->flags &= any_proxy::stale_flag;
	}

	bool test_stale () const
	{
		return ( ( this->flags & any_proxy::stale_flag ) != 0 );
	}


	void set_dead ()
	{
		this->flags &= any_proxy::dead_flag;
	}

	bool test_dead () const
	{
		return ( ( this->flags & any_proxy::dead_flag ) != 0 );
	}


	void link_to ( any_object* op );  // not join
};



struct any_object
/*
	Default construction doesn't set type but does set
	null flag. Subtypes must call set_type() from their 
	default constructors.

	eval and upd constraint flags are mutually exclusive.
*/
{
	enum
	{
		null_flag	= 0x01,
		eval_flag	= 0x02,
		upd_flag	= 0x04
	};

	type*				p_type;		// type record
	any_proxy*			p_proxy;	// linked proxy list
	unsigned char		flags;		// null, eval, upd


	any_object ()
	{
		this->p_type = nullptr;
		this->p_proxy = nullptr;
		this->flags = null_flag;
	}

	~any_object ();


	void set_type ( type* pt ) 
	{
		this->p_type = pt;
	}

	bool test_type ( type* pt ) const
	{
		return ( this->p_type == pt );
	}


	void set_null ()
	{
		this->flags &= any_object::null_flag;
	}

	void clear_null ()
	{
		this->flags &= ~any_object::null_flag;
	}
	
	bool test_null () const
	{
		return ( ( this->flags & any_object::null_flag ) != 0 );
	}


	void set_eval () 
	{
		this->flags |= any_object::eval_flag;
		this->flags &= ~any_object::upd_flag;
	}

	bool test_eval () const
	{
		return ( ( this->flags & any_object::eval_flag ) != 0 );
	}


	void set_upd () 
	{
		this->flags |= any_object::upd_flag;
		this->flags &= ~any_object::eval_flag;
	}

	bool test_upd () const
	{
		return ( ( this->flags & any_object::upd_flag ) != 0 );
	}


	void set_free ()  // remove constraints
	{
		this->flags &= ~( any_object::eval_flag | any_object::upd_flag );
	}


	int get_access () const
	{
		return ( this->test_eval() ? 0 : ( this->test_upd() ? 1 : 2 ) );
	}


	void add_dependent ( any_proxy* pp )
	{
		pp->link_to( this );
	}

	any_object* find_constituent ( type* pt );  // recursive
};



template< class ot > struct oref
/*
	General purpose typed object ref.

	oref<> is a base type and an actual type.  When used
	as an actual type, it necessarily references an actual 
	Nod	type-object, which is neccessarily unconstrained. A 
	stand-alone oref<> is usually used to give imo to a Nod 
	procedure function with [init] intent.

	if p_typed is null
		ref is void
	else if p_any is null
		p_typed points to typed actual object
	else
		p_typed points to typed base sub-object
		p_any points to actual object, type tbd

	resolve() returns pointer to untyped actual object.

	cast_near() translates a ref to a given base type,
	starting at the oref's typed constituent.

	Note: This function is called directly by generated 
	code to bypass a procedure function call that would
	otherwise implement Nod translator method :as_base<t>.

	cast_far() translates a ref to a given type starting
	from the oref's actual object. The cast may result is a
	void oref. 
	
	Note: This function is called by Nod procedure function
	essential<>::as_t_ which imlpements Nod translator 
	method :as<t>.
	

	restore() removes any constraints applied by a con-
	version	to irefu<> or iref<>.
*/
{
	ot*				p_typed;
	any_object*		p_any; 

	oref ( ot* p_obj )  // actual object
	{
		this->p_typed = p_obj;
		this->p_any = nullptr;
	}

	oref ( oref<ot> const& other ) // copy 
	{
		this->p_typed = other.p_typed;
		this->p_any = other.p_any;
	}

	any_object* resolve () const  // return actual object 
	{
		return ( ( this->p_any == nullptr ) ? this->p_typed : this->p_any );
	}

	bool cast_near ( type* pt )  // verified base translation
	{
		if ( this->p_any == nullptr ) this->p_any = this->p_typed;
		this->p_typed = (ot*)this->p_typed->find_constituent( pt );  // from sub-object
		return ( this->p_typed != nullptr );
	}

	bool cast_far ( type* pt ) // unverified random translation
	{
		if ( this->p_any == nullptr ) this->p_any = this->p_typed;
		this->p_typed = (ot*)this->p_any->find_constituent( pt );  // from actual obect
		return ( this->p_typed != nullptr );
	}

	bool test () const // not void
	{
		return ( this->p_typed != nullptr );
	}

	void restore ()
	{
		this->p_typed->set_free();
	}
};


template< class ot > struct irefu : public oref<ot>
/*
	marked input ref - applies [upd] constraint

	HNM CALL must pass an irefu<> for every marked input in
	a Nod procedure function call.

	If an irefu<> is created from an object pointer p_obj,
	the [upd] constraint must be reversed after the Nod
	procedure call by calling p_object->set_free(). 
	
	If an irefu<>is created by converting an oref<>, the 
	[upd] constraint must be reversed after the Nod proce-
	dure call by calling oref<>::restore().
	
	Note! Don't call this restore() to reverse the con-
	straint.

*/
{
	irefu ( ot* p_obj ) // actual object
	:oref<ot> ( p_obj )
	{
		this->p_typed->set_upd();
	}

	irefu ( oref<ot> const& other ) // constrained conversion 
	:oref<ot> ( other )
	{
		this->p_typed->set_upd();
	}

	irefu ( irefu<ot> const& other ) // copy
	:oref<ot> ( other )
	{
	}

	void restore () const // restore upd constraint
	{
		this->p_typed->set_upd();
	}
};


template< class ot > struct iref : public oref<ot>
/*
	default input ref - applies [eval] constraint

	HNM CALL must pass an iref<> for every regular input in
	a Nod procedure function call.

	If an iref<> is created from an object pointer p_obj,
	the [eval] constraint must be reversed after the Nod
	procedure call by calling p_object->set_free(). 
	
	If an iref<>is created by converting an oref<>, the 
	[eval] constraint must be reversed after the Nod proce-
	dure call by calling oref<>::restore().

	If an iref<>is created by converting an irefu<>, the 
	[eval] constraint must be reversed after the Nod proce-
	dure call by calling irefu<>::restore().

*/
{
	iref ( ot* p_obj )  // actual object
	:oref<ot> ( p_obj )
	{
		this->p_typed->set_eval();
	}

	iref ( oref<ot> const& other ) // constrained conversion
	:oref<ot> ( other )
	{
		this->p_typed->set_eval();
	}

	iref ( irefu<ot> const& other ) // constrained conversion 
	:oref<ot> ( other )
	{
		this->p_typed->set_eval();
	}

	iref ( iref<ot> const& other ) // copy
	:oref<ot> ( other )
	{
	}
};



template< class ot > struct proxy : public any_proxy
/*
	After default construction, a proxy must be joined
	and constrained as needed. 

	HNM JOIN generates the call to join().
	
	If proxy access is verified and known at compile time,
	the constraint function is called immediately after	the
	generated proxy declaration.
	
	Otherwise, the Nod proxy def will indicate runtime ver-
	ification, and the constraint must be set and verified
	by HNM CALL or HNM JOIN.

	When a proxy is used as an input, it must be converted
	to an iref<> or irefu<>.  The conversion captures an 
	access undo level.  After the Nod procedure function 
	returns, HNM CALL must call restore() to revert the 
	constraint as needed.  
	
	Note: restore() doesn't change the intrinsic proxy 
	constraint.
*/
{
	oref<ot>	link;	// joined object
	int			undo;	// access level 0,1,2

	proxy ()  // void
	{
		this->undo = 0;
	}

	~proxy ()
	{
		//  See any_proxy notes about deconstruction. 
		if ( this->p_prev == nullptr && this->link.p_typed->p_proxy == this ) this->link.p_typed->p_proxy = this->p_next;  // may be nullptr
	}

	void join ( ot* p_obj ) // actual object
	{
		this->link.p_typed = p_obj;
	}

	void join ( proxy<ot>* p_other )  // copy 
	{
		this->link = p_other->link;
	}

	bool verify () const
	{
		return ( this->get_access() <= link.p_typed->get_access() ); 
	}

	iref<ot> as_iref () 
	{
		int level = this->link.p_typed->get_access();
		if ( level > 0 ) this->undo = level;
		return iref<ot> ( this->link );
	}

	irefu<ot> as_irefu ()
	{
		int level = this->link.p_typed->get_access();
		if ( level > 1 ) this->undo = level;
		return iref<ot> ( this->link );
	}

	ot* as_output () 
	{
		return ( ot* )this->link.resolve();  
	}

	void restore ()
	{
		if ( this->undo == 1 ) this->link.p_typed->set_upd();
		else if ( this->undo == 2 ) this->link.p_typed->set_free();
		this->undo = 0;
	}

	bool test_void () const
	{
		return ( this->link.p_typed == nullptr );
	}

	bool test_good () const
	{
		return ( !this->test_void() && !this->test_stale() && !this->test_dead() );
	}
};



const char* get_c_name ( const char* p_nod_name, std::basic_stringstream<char>& ss );


