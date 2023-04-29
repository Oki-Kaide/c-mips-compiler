#ifndef IR_VARIABLE_MAP_H
#define IR_VARIABLE_MAP_H

#include <map>

#include "Type.hpp"

struct Binding {
	std::string alias;
	Type type;
	bool is_global;
	bool is_function;
	std::vector<Type> params;

	Binding() {}

	Binding(std::string alias, Type type, bool is_global)
	: alias(alias), type(type), is_global(is_global), is_function(false) {
	}

	Binding(std::string alias, Type type, std::vector<Type> const& params)
	: alias(alias), type(type), is_global(true), is_function(true) {
		this->params = params;
	}
};

// **********************************

struct ArrayType {
	Type type;
	unsigned elements;

	ArrayType() {}

	ArrayType(Type type, unsigned elements)
	: type(type), elements(elements) {
	}

	unsigned stride() const {
		if(type.bytes() == 1 || type.bytes() == 2 || type.bytes() == 4) {
			return type.bytes();
		} else {
			unsigned s = type.bytes();
			align_address(s, 4);
			return s;
		}
	}

	unsigned total_size() const {
		return stride() * elements;
	}

};

typedef std::map<std::string, ArrayType> ArrayMap;

// **********************************

class Scope;

struct StructureType {
	std::map<std::string, Type> members;
	ArrayMap arrays;
	std::vector<std::string> order;

	unsigned total_size() const {
		return get_member_offset(order.back()) + get_member_type(order.back()).bytes();
	}

	bool member_exists(std::string) const;
	Type get_member_type(std::string) const;
	unsigned get_member_offset(std::string) const;

	void add_members(Scope* scope);
	void Debug(std::ostream& dst) const;
};

class StructureMap : public std::map<std::string, StructureType> {
public:
	void add(std::string name, StructureType s);
	void print(std::ostream& dst) const;
};

StructureMap structures();

unsigned struct_total_size(std::string name);

// **********************************

struct EnumType {
	EnumType();

	std::map<std::string, int> members;
	int next_member;

	bool member_exists(std::string name) const;
	int get_member_value(std::string name) const;

	void add(std::string name);
	void add(std::string name, int value);
	void Debug(std::ostream& dst) const;
};

class EnumMap : public std::map<std::string, EnumType> {
public:
	void add(std::string name, EnumType s);
	void print(std::ostream& dst) const;

	bool value_exists(std::string name) const;
	int get_value(std::string name) const;
};

EnumMap enums();

struct enumerator_entry {
	// temporary structure used for parser only
	std::string name;
	int val;
	bool with_val;
	enumerator_entry(std::string, int, bool);
};

// **********************************

void typedefs_define(std::string alias, Type type);
bool typedefs_exists(std::string alias);
Type typedefs_get(std::string alias);

// **********************************

class Declaration;

class VariableMap : public std::map<std::string, Binding> {
public:
	std::string break_destination;
	std::string continue_destination;

	VariableMap();

	void add_bindings(std::vector<Declaration*> const& declarations);
};


// **********************************

class FunctionStack : public std::map<std::string, Type> {
public:
	ArrayMap arrays;
	void add_variables(VariableMap const& aliases, std::vector<Declaration*> const& declarations);
};


#endif
