#include "Instruction.hpp"
#include "Conversions.hpp"
#include "UniqueNames.hpp"

#include <iomanip>
#include <sstream>

void Instruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	out << "    undefined\n";
}

// *******************************************

LabelInstruction::LabelInstruction(std::string name) : label_name(name) {}

void LabelInstruction::Debug(std::ostream &dst) const {
	dst << "  " << label_name << ":" << std::endl;
}

void LabelInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	out << "  " << label_name << ":\n";
}

// *******************************************

GotoInstruction::GotoInstruction(std::string name) : label_name(name) {}

void GotoInstruction::Debug(std::ostream &dst) const {
	dst << "    goto " << label_name << std::endl;
}

void GotoInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	out << "    j       " << label_name << "\n";
	out << "    nop\n";
}

// *******************************************

GotoIfEqualInstruction::GotoIfEqualInstruction(std::string name, std::string variable, int32_t value) : label_name(name), variable(variable), value(value) {}

void GotoIfEqualInstruction::Debug(std::ostream &dst) const {
	dst << "    beq " << variable << ", " << value << ", " << label_name << std::endl;
}

void GotoIfEqualInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	std::string skip_label = unique("$L");
	context.load_variable(out, variable, 8);
	convert_type(out, 8, context.get_type(variable), 10, Type("int", 0));
	if(value == 0) {
		out << "    bne     $10, $0, " << skip_label << "\n";
	} else {
		out << "    li      $11, " << value << "\n";
		out << "    bne     $10, $11, " << skip_label << "\n";
	}
	out << "    nop\n";
	out << "    j       " << label_name << "\n";
	out << "    nop\n";
	out << "   " << skip_label << ":\n";
}

// *******************************************

ReturnInstruction::ReturnInstruction() : return_variable("") {}
ReturnInstruction::ReturnInstruction(std::string return_variable) : return_variable(return_variable) {}

void ReturnInstruction::Debug(std::ostream &dst) const {
	dst << "    return " << return_variable << std::endl;
}

void ReturnInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	if(return_variable != "") {
		if(context.get_return_type().is_struct()) {
			// make sure the structs are equal
			if(!context.get_return_type().equals(context.get_type(return_variable))) {
				throw compile_error((std::string)"type mismatch: cannot return a variable of type '" + context.get_type(return_variable).name() + "' in a function of type '" + context.get_return_type().name() + "'");
			}
			// get the base address of the struct
			out << "    lw      $3, " << context.get_return_struct_offset() << "($fp)\n";
			out << "    nop\n";
			// copy the struct into the address
			context.copy(out, return_variable, "", context.get_return_type().bytes());
		} else {
			// populate register $2 with return value
			context.load_variable(out, return_variable, 8);
			convert_type(out, 8, context.get_type(return_variable), 2, context.get_return_type());
			// copy it to FPU if float
			if(context.get_return_type().is_float()) {
				if(context.get_return_type().bytes() == 4) {
					out << "    mtc1    $2, $f0\n";
				} else {
					out << "    sw      $2, 0($fp)\n";
					out << "    sw      $3, 4($fp)\n";
					out << "    ldc1    $f0, 0($fp)\n";
				}
			}
		}
	}
	out << "    j       " << context.get_return_label() << "\n";
	out << "    nop\n";
}

// *******************************************

ConstantInstruction::ConstantInstruction(std::string destination, Type type, uint32_t dataLo, uint32_t dataHi)
: destination(destination), type(type), dataLo(dataLo), dataHi(dataHi) {}

void ConstantInstruction::Debug(std::ostream &dst) const {
	dst << "    constant " << destination
	<< " "
	<< std::hex << std::setfill('0') << std::setw(8) << dataHi
	<< " "
	<< std::hex << std::setfill('0') << std::setw(8) << dataLo
	<< std::dec << std::endl;
}

void ConstantInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	if(type.bytes() == 8) {
		out << "    li      $8, " << dataHi <<"\n";
		out << "    li      $9, " << dataLo <<"\n";
	} else {
		if(dataLo == 0) {
			out << "    move    $8, $0\n";
		} else {
			out << "    li      $8, " << dataLo <<"\n";
		}
	}
	context.store_variable(out, destination, 8);
}

std::string very_conservative_escape(std::string src) {
	std::stringstream ss;
	for(unsigned i = 0; i < src.size(); i++) {
		char c = src[i];
		if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == ' ') {
			ss << c;
		} else {
			ss << "\\" << std::setfill('0') << std::setw(3) << std::oct << (int)c;
		}
	}
	return ss.str();
}

StringInstruction::StringInstruction(std::string destination, std::string data)
: destination(destination), data(data) {}

void StringInstruction::Debug(std::ostream &dst) const {
	dst << "    ascii " << destination << " " << data << std::endl;
}

void StringInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	std::stringstream ss;
	buff << "    .align 2\n";
	buff << "  string_data_" << destination << ":\n";
	buff << "    .ascii \"" << very_conservative_escape(data) << "\\000\"\n";
	out << "    lui     $8, %hi(string_data_" << destination << ")\n";
	out << "    addiu   $8, $8, %lo(string_data_" << destination << ")\n";
	out << "    nop\n";
	context.store_variable(out, destination, 8);
}

// *******************************************

MoveInstruction::MoveInstruction(std::string destination, std::string source)
: destination(destination), source(source) {}

void MoveInstruction::Debug(std::ostream &dst) const {
	dst << "    move " << destination << ", " << source << std::endl;
}

void MoveInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	if(context.get_type(destination).is_struct() && context.get_type(destination).equals(context.get_type(source))) {
		// do a byte-wise copy
		context.copy(out, source, destination, context.get_type(destination).bytes());
	} else {
		// do a conversion
		context.load_variable(out, source, 8);
		convert_type(out, 8, context.get_type(source), 10, context.get_type(destination));
		context.store_variable(out, destination, 10);
	}
}

AssignInstruction::AssignInstruction(std::string destination, std::string source)
: destination(destination), source(source) {}

void AssignInstruction::Debug(std::ostream &dst) const {
	dst << "    assign *" << destination << ", " << source << std::endl;
}

void AssignInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	// get the address of the destination to assign
	context.load_variable(out, destination, 3);

	if(context.get_type(destination).dereference().is_struct() && context.get_type(destination).dereference().equals(context.get_type(source))) {
		// do a byte-wise copy
		context.copy(out, source, "", context.get_type(destination).dereference().bytes());
	} else {
		// do a conversion
		context.load_variable(out, source, 8);
		convert_type(out, 8, context.get_type(source), 10, context.get_type(destination).dereference());
		switch (context.get_type(destination).dereference().bytes()) {
			case 1:
				out << "    sb      $10, 0($3)\n"; break;
			case 2:
				out << "    sh      $10, 0($3)\n"; break;
			case 4:
				out << "    sw      $10, 0($3)\n"; break;
			case 8:
				out << "    sw      $10, 0($3)\n";
				out << "    sw      $11, 4($3)\n";
				break;
		}
		out << "    nop\n";
	}
}

// *******************************************

AddressOfInstruction::AddressOfInstruction(std::string destination, std::string source)
: destination(destination), source(source) {}

void AddressOfInstruction::Debug(std::ostream &dst) const {
	dst << "    addressOf " << destination << ", &" << source << std::endl;
}

void AddressOfInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	if(context.is_global(source)) {
		out << "    lui     $8, %hi(" << source << ")\n";
		out << "    addiu   $8, $8, %lo(" << source << ")\n";
	} else {
		out << "    addiu   $8, $fp, " << context.get_stack_offset(source) << "\n";
	}
	context.store_variable(out, destination, 8);
}

DereferenceInstruction::DereferenceInstruction(std::string destination, std::string source)
: destination(destination), source(source) {}

void DereferenceInstruction::Debug(std::ostream &dst) const {
	dst << "    dereference " << destination << ", *" << source << std::endl;
}

void DereferenceInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	if(context.is_global(source)) {
		out << "    lui     $2, %hi(" << source << ")\n";
		out << "    addiu   $2, $2, %lo(" << source << ")\n";
		out << "    lw      $2, 0($2)\n";
	} else {
		out << "    lw      $2, " << context.get_stack_offset(source) << "($fp)\n";
	}
	out << "    nop\n";
	context.copy(out, "", destination, context.get_type(destination).bytes());
}

// *******************************************

LogicalInstruction::LogicalInstruction(std::string destination, std::string source1, std::string source2, char logicalType)
: destination(destination), source1(source1), source2(source2), logicalType(logicalType) {}

void LogicalInstruction::Debug(std::ostream &dst) const {
	switch (logicalType) {
		case '&':
			dst << "    logicalAnd " << destination << ", " << source1 << ", " << source2 << std::endl;
			break;
		case '|':
			dst << "    logicalOr " << destination << ", " << source1 << ", " << source2 << std::endl;
			break;
		case '!':
			dst << "    logicalNot " << destination << ", " << source1 << std::endl;
			break;
		default:
			throw compile_error("unsupported type of boolean operator in LogicalInstruction");
	}
}

void LogicalInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	// load and convert to ints: in $10 and $13
	context.load_variable(out, source1, 8);
	convert_type(out, 8, context.get_type(source1), 10, Type("int", 0));
	if(logicalType != '!') {
		context.load_variable(out, source2, 11);
		convert_type(out, 11, context.get_type(source2), 13, Type("int", 0));
	}
	std::string skip_label1 = unique("$L");
	std::string skip_label2 = unique("$L");
	switch (logicalType) {
		case '&':
			// if either is zero, return 0
			out << "    li      $14, 1\n";
			out << "    bne     $10, $0, " << skip_label1 << "\n";
			out << "    nop\n";
			out << "    li      $14, 0\n";
			out << "   " << skip_label1 << ":\n";
			out << "    bne     $13, $0, " << skip_label2 << "\n";
			out << "    nop\n";
			out << "    li      $14, 0\n";
			out << "   " << skip_label2 << ":\n";
			break;
		case '|':
			// if either is non-zero, return 1
			out << "    li      $14, 0\n";
			out << "    beq     $10, $0, " << skip_label1 << "\n";
			out << "    nop\n";
			out << "    li      $14, 1\n";
			out << "   " << skip_label1 << ":\n";
			out << "    beq     $13, $0, " << skip_label2 << "\n";
			out << "    nop\n";
			out << "    li      $14, 1\n";
			out << "   " << skip_label2 << ":\n";
			break;
		case '!':
			// if source is zero, return 1, else return 0
			out << "    li      $14, 0\n";
			out << "    bne     $10, $0, " << skip_label1 << "\n";
			out << "    nop\n";
			out << "    li      $14, 1\n";
			out << "   " << skip_label1 << ":\n";
			break;
		default:
			throw compile_error("unsupported type of boolean operator in LogicalInstruction");
	}
	context.store_variable(out, destination, 14);
}

// *******************************************

BitwiseInstruction::BitwiseInstruction(std::string destination, std::string source1, std::string source2, char operatorType)
: destination(destination), source1(source1), source2(source2), operatorType(operatorType) {}

void BitwiseInstruction::Debug(std::ostream &dst) const {
	switch (operatorType) {
		case '&':
			dst << "    bitwiseAnd " << destination << ", " << source1 << ", " << source2 << std::endl;
			break;
		case '|':
			dst << "    bitwiseOr " << destination << ", " << source1 << ", " << source2 << std::endl;
			break;
		case '^':
			dst << "    bitwiseXor " << destination << ", " << source1 << ", " << source2 << std::endl;
			break;
		case '~':
			dst << "    bitwiseNot " << destination << ", " << source1 << std::endl;
			break;
		default:
			throw compile_error("unsupported type of boolean operator in BitwiseInstruction");
	}
}

void BitwiseInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	if(!context.get_type(source1).is_integer()) {
		throw compile_error("cannot perform a bitwise operation on structs, unions, or floats");
	}
	if(operatorType != '~') {
		if(!context.get_type(source2).is_integer()) {
			throw compile_error("cannot perform a bitwise operation on structs, unions, or floats");
		}
	}
	// load up our integers
	context.load_variable(out, source1, 8);
	if(operatorType != '~') context.load_variable(out, source2, 9);
	// perform the bitwise operation and store into $10
	switch (operatorType) {
		case '&':
			out << "    and     $10, $8, $9\n";
			break;
		case '|':
			out << "    or      $10, $8, $9\n";
			break;
		case '^':
			out << "    xor     $10, $8, $9\n";
			break;
		case '~':
			out << "    not     $10, $8\n";
			break;
		default:
			throw compile_error("unsupported type of boolean operator in BitwiseInstruction");
	}
	// store back into memory
	context.store_variable(out, destination, 10);
}

// *******************************************

EqualityInstruction::EqualityInstruction(std::string destination, std::string source1, std::string source2, char equalityType)
: destination(destination), source1(source1), source2(source2), equalityType(equalityType) {}

void EqualityInstruction::Debug(std::ostream &dst) const {
	switch (equalityType) {
		case '=':
			dst << "    equals " << destination << ", " << source1 << ", " << source2 << std::endl;
			break;
		case '!':
			dst << "    notEquals " << destination << ", " << source1 << ", " << source2 << std::endl;
			break;
		case '<':
			dst << "    lessThan " << destination << ", " << source1 << ", " << source2 << std::endl;
			break;
		case '>':
			dst << "    greaterThan " << destination << ", " << source1 << ", " << source2 << std::endl;
			break;
		case 'l':
			dst << "    lessOrEq " << destination << ", " << source1 << ", " << source2 << std::endl;
			break;
		case 'g':
			dst << "    greaterOrEq " << destination << ", " << source1 << ", " << source2 << std::endl;
			break;
		default:
			throw compile_error("unsupported type of relational operator in EqualityInstruction");
	}
}

void fpu_comparison_instruction(std::ostream& out, char equalityType, std::string floattype, std::string skip_label) {
	switch (equalityType) {
		case '=':
			out << "    c.eq." << floattype << "  $f0, $f2\n";
			out << "    bc1t    " << skip_label << "\n";
			break;
		case '!':
			out << "    c.eq." << floattype << "  $f0, $f2\n";
			out << "    bc1f    " << skip_label << "\n";
			break;
		case '<':
			out << "    c.lt." << floattype << "  $f0, $f2\n";
			out << "    bc1t    " << skip_label << "\n";
			break;
		case '>':
			out << "    c.lt." << floattype << "  $f2, $f0\n";
			out << "    bc1t    " << skip_label << "\n";
			break;
		case 'l':
			out << "    c.le." << floattype << "  $f0, $f2\n";
			out << "    bc1t    " << skip_label << "\n";
			break;
		case 'g':
			out << "    c.le." << floattype << "  $f2, $f0\n";
			out << "    bc1t    " << skip_label << "\n";
			break;
		default:
			throw compile_error("unsupported type of relational operator in EqualityInstruction");
	}
}

void EqualityInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	out << "    li      $24, 1\n";
	std::string skip_label = unique("$L");
	Type l = context.get_type(source1);
	Type r = context.get_type(source2);
	if((l.is_integer() || l.is_pointer()) && (r.is_integer() || r.is_pointer())) {
		context.load_variable(out, source1, 8);
		context.load_variable(out, source2, 9);
		convert_type(out, 8, l, 10, Type("int", 0));
		convert_type(out, 9, r, 11, Type("int", 0));
		switch (equalityType) {
			case '=':
				out << "    beq     $10, $11, " << skip_label << "\n";
				break;
			case '!':
				out << "    bne     $10, $11, " << skip_label << "\n";
				break;
			case '<':
				if(l.is_signed() && r.is_signed()) {
					out << "    slt     $25, $10, $11\n";
				} else {
					out << "    sltu    $25, $10, $11\n";
				}
				out << "    bne     $25, $0, " << skip_label << "\n";
				break;
			case '>':
				if(l.is_signed() && r.is_signed()) {
					out << "    slt     $25, $11, $10\n";
				} else {
					out << "    sltu    $25, $11, $10\n";
				}
				out << "    bne     $25, $0, " << skip_label << "\n";
				break;
			case 'l':
				if(l.is_signed() && r.is_signed()) {
					out << "    slt     $25, $11, $10\n";
				} else {
					out << "    sltu    $25, $11, $10\n";
				}
				out << "    beq     $25, $0, " << skip_label << "\n";
				break;
			case 'g':
				if(l.is_signed() && r.is_signed()) {
					out << "    slt     $25, $10, $11\n";
				} else {
					out << "    sltu    $25, $10, $11\n";
				}
				out << "    beq     $25, $0, " << skip_label << "\n";
				break;
			default:
				throw compile_error("unsupported type of relational operator in EqualityInstruction");
		}

	} else if(l.is_float() && r.is_integer()) {
		context.load_variable(out, source1, 12);
		context.load_variable(out, source2, 8);
		convert_type(out, 8, r, 14, l);
		if(l.bytes() == 4) {
			out << "    mtc1    $12, $f0\n";
			out << "    mtc1    $14, $f2\n";
			fpu_comparison_instruction(out, equalityType, "s", skip_label);
		} else {
			out << "    sw      $12, 0($fp)\n";
			out << "    sw      $13, 4($fp)\n";
			out << "    ldc1    $f0, 0($fp)\n";
			out << "    sw      $14, 0($fp)\n";
			out << "    sw      $15, 4($fp)\n";
			out << "    ldc1    $f2, 0($fp)\n";
			fpu_comparison_instruction(out, equalityType, "d", skip_label);
		}

	} else if(l.is_integer() && r.is_float()) {
		context.load_variable(out, source1, 8);
		convert_type(out, 8, l, 12, r);
		context.load_variable(out, source2, 14);
		if(l.bytes() == 4) {
			out << "    mtc1    $12, $f0\n";
			out << "    mtc1    $14, $f2\n";
			fpu_comparison_instruction(out, equalityType, "s", skip_label);
		} else {
			out << "    sw      $12, 0($fp)\n";
			out << "    sw      $13, 4($fp)\n";
			out << "    ldc1    $f0, 0($fp)\n";
			out << "    sw      $14, 0($fp)\n";
			out << "    sw      $15, 4($fp)\n";
			out << "    ldc1    $f2, 0($fp)\n";
			fpu_comparison_instruction(out, equalityType, "d", skip_label);
		}

	} else if(l.is_float() && r.is_float()) {
		context.load_variable(out, source1, 8);
		context.load_variable(out, source2, 10);
		if(l.bytes() == 8 || r.bytes() == 8) {
			convert_type(out, 8, l, 12, Type("double", 0));
			convert_type(out, 10, r, 14, Type("double", 0));
			out << "    sw      $12, 0($fp)\n";
			out << "    sw      $13, 4($fp)\n";
			out << "    ldc1    $f0, 0($fp)\n";
			out << "    sw      $14, 0($fp)\n";
			out << "    sw      $15, 4($fp)\n";
			out << "    ldc1    $f2, 0($fp)\n";
			fpu_comparison_instruction(out, equalityType, "d", skip_label);
		} else {
			out << "    mtc1    $8, $f0\n";
			out << "    mtc1    $10, $f2\n";
			fpu_comparison_instruction(out, equalityType, "s", skip_label);
		}

	} else {
		throw compile_error((std::string)"relational operator not defined between types '" + l.name() + "' and '" + r.name() + "'");
	}
	out << "    nop\n";
	out << "    li      $24, 0\n";
	out << "   " << skip_label << ":\n";
	context.store_variable(out, destination, 24);
}

// *******************************************

ShiftInstruction::ShiftInstruction(std::string destination, std::string source1, std::string source2, bool doRightShift)
: destination(destination), source1(source1), source2(source2), doRightShift(doRightShift) {}

void ShiftInstruction::Debug(std::ostream &dst) const {
	if(doRightShift) {
		dst << "    rightshift " << destination << ", " << source1 << ", " << source2 << std::endl;
	} else {
		dst << "    leftshift " << destination << ", " << source1 << ", " << source2 << std::endl;
	}
}

void ShiftInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	if(!context.get_type(source1).is_integer() || !context.get_type(source1).is_integer()) {
		throw compile_error("cannot perform a shift operation on structs, unions, or floats");
	}
	context.load_variable(out, source1, 8);
	context.load_variable(out, source2, 9);
	if(doRightShift) {
		if(context.get_type(source1).is_signed()) {
			out << "    srav    $10, $8, $9\n";
		} else {
			out << "    srlv    $10, $8, $9\n";
		}
	} else {
		out << "    sllv    $10, $8, $9\n";
	}
	context.store_variable(out, destination, 10);
}

// *******************************************

NegativeInstruction::NegativeInstruction(std::string destination, std::string source)
: destination(destination), source(source) {}

void NegativeInstruction::Debug(std::ostream &dst) const {
	dst << "    negative " << destination << ", " << source << std::endl;
}

void NegativeInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	if(context.get_type(source).is_struct()) {
		throw compile_error("cannot make structs or unions negative");
	}
	if(context.get_type(source).is_integer()) {
		context.load_variable(out, source, 8);
		out << "    subu    $10, $0, $8\n";
		context.store_variable(out, destination, 10);
	} else if(context.get_type(source).bytes() == 4) {
		context.load_variable(out, source, 12);
		out << "    mtc1    $12, $f0\n";
		out << "    neg.s   $f0, $f0\n";
		out << "    mfc1    $8, $f0\n";
		context.store_variable(out, destination, 8);
	} else {
		context.load_variable(out, source, 12);
		out << "    sw      $12, 0($fp)\n";
		out << "    sw      $13, 4($fp)\n";
		out << "    ldc1    $f0, 0($fp)\n";
		out << "    neg.d   $f0, $f0\n";
		out << "    sdc1    $f0, 0($fp)\n";
		out << "    lw      $8, 0($fp)\n";
		out << "    lw      $9, 4($fp)\n";
		out << "    nop\n";
		context.store_variable(out, destination, 8);
	}
}

// *******************************************

void float_operation(std::string type, std::ostream& out) {
	out << "    mtc1    $12, $f0\n";
	out << "    mtc1    $14, $f2\n";
	out << "    " << type << ".s   $f0, $f0, $f2\n";
	out << "    mfc1    $8, $f0\n";
}

void double_operation(std::string type, std::ostream& out) {
	out << "    sw      $12, 0($fp)\n";
	out << "    sw      $13, 4($fp)\n";
	out << "    ldc1    $f0, 0($fp)\n";
	out << "    sw      $14, 0($fp)\n";
	out << "    sw      $15, 4($fp)\n";
	out << "    ldc1    $f2, 0($fp)\n";
	out << "    " << type << ".d   $f0, $f0, $f2\n";
	out << "    sdc1    $f0, 0($fp)\n";
	out << "    lw      $8, 0($fp)\n";
	out << "    lw      $9, 4($fp)\n";
	out << "    nop\n";
}

// *******************************************

IncrementInstruction::IncrementInstruction(std::string destination, std::string source, bool decrement)
: destination(destination), source(source), decrement(decrement) {}

void IncrementInstruction::Debug(std::ostream &dst) const {
	if(decrement) {
		dst << "    decrement " << destination << ", " << source << std::endl;
	} else {
		dst << "    increment " << destination << ", " << source << std::endl;
	}
}

void IncrementInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	if(context.get_type(source).is_struct()) {
		throw compile_error("cannot increment/decrement structs or unions");
	}
	if(context.get_type(source).is_integer()) {
		// regular integer
		context.load_variable(out, source, 8);
		if(decrement) {
			out << "    addiu   $10, $8, -1\n";
		} else {
			out << "    addiu   $10, $8, 1\n";
		}
		context.store_variable(out, destination, 10);

	} else if(context.get_type(source).is_pointer()) {
		// increment pointer
		context.load_variable(out, source, 8);
		if(decrement) {
			out << "    addiu   $10, $8, -" << context.get_type(source).dereference().bytes() << "\n";
		} else {
			out << "    addiu   $10, $8, " << context.get_type(source).dereference().bytes() << "\n";
		}

	} else if(context.get_type(source).bytes() == 4) {
		// float
		context.load_variable(out, source, 12);
		out << "    li      $14, 0x3f800000\n";
		float_operation(decrement ? "sub" : "add", out);
		context.store_variable(out, destination, 8);

	} else {
		// double
		context.load_variable(out, source, 12);
		out << "    li      $14, 0x3ff00000\n";
		out << "    move    $15, $0\n";
		double_operation(decrement ? "sub" : "add", out);
		context.store_variable(out, destination, 8);
	}
}

// *******************************************

AddInstruction::AddInstruction(std::string destination, std::string source1, std::string source2)
: destination(destination), source1(source1), source2(source2) {}

void AddInstruction::Debug(std::ostream &dst) const {
	dst << "    add " << destination << ", " << source1 << ", " << source2 << std::endl;
}

void AddInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	// load the two operands in registers
	context.load_variable(out, source1, 8);
	context.load_variable(out, source2, 10);

	// special case: pointer arithmetic
	if(context.get_type(source1).is_pointer()) {
		if(context.get_type(source2).is_integer()) {
			out << "    li      $11, " << context.get_type(source1).dereference().bytes() << "\n";
			out << "    mul     $10, $10, $11\n";
		}
		out << "    addu    $14, $8, $10\n";
		context.store_variable(out, destination, 14);
		return;
	}
	if(context.get_type(source2).is_pointer()) {
		out << "    li      $11, " << context.get_type(source2).dereference().bytes() << "\n";
		out << "    mul     $8, $8, $11\n";
		out << "    addu    $14, $8, $10\n";
		context.store_variable(out, destination, 14);
		return;
	}

	// convert them to the destination type
	Type result_type = context.get_type(destination);
	convert_type(out, 8, context.get_type(source1), 12, result_type);
	convert_type(out, 10, context.get_type(source2), 14, result_type);
	// perform the add
	if(result_type.is_integer()) {
		out << "    addu    $8, $12, $14\n";
		context.store_variable(out, destination, 8);
	} else if(result_type.bytes() == 4) {
		float_operation("add", out);
		context.store_variable(out, destination, 8);
	} else {
		double_operation("add", out);
		context.store_variable(out, destination, 8);
	}
}

// *******************************************

SubInstruction::SubInstruction(std::string destination, std::string source1, std::string source2)
: destination(destination), source1(source1), source2(source2) {}

void SubInstruction::Debug(std::ostream &dst) const {
	dst << "    sub " << destination << ", " << source1 << ", " << source2 << std::endl;
}

void SubInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	// load the two operands in registers
	context.load_variable(out, source1, 8);
	context.load_variable(out, source2, 10);
	// special case: pointer arithmetic
	if(context.get_type(source1).is_pointer()) {
		if(context.get_type(source2).is_integer()) {
			out << "    li      $11, " << context.get_type(source1).dereference().bytes() << "\n";
			out << "    mul     $10, $10, $11\n";
		}
		out << "    subu    $14, $8, $10\n";
		context.store_variable(out, destination, 14);
		return;
	}
	// convert them to the destination type
	Type result_type = context.get_type(destination);
	convert_type(out, 8, context.get_type(source1), 12, result_type);
	convert_type(out, 10, context.get_type(source2), 14, result_type);
	// perform the subtraction
	if(result_type.is_integer()) {
		out << "    subu    $8, $12, $14\n";
		context.store_variable(out, destination, 8);
	} else if(result_type.bytes() == 4) {
		float_operation("sub", out);
		context.store_variable(out, destination, 8);
	} else {
		double_operation("sub", out);
		context.store_variable(out, destination, 8);
	}
}

// *******************************************

MulInstruction::MulInstruction(std::string destination, std::string source1, std::string source2)
: destination(destination), source1(source1), source2(source2) {}

void MulInstruction::Debug(std::ostream &dst) const {
	dst << "    mul " << destination << ", " << source1 << ", " << source2 << std::endl;
}

void MulInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	// load the two operands in registers
	context.load_variable(out, source1, 8);
	context.load_variable(out, source2, 10);
	// convert them to the destination type
	Type result_type = context.get_type(destination);
	convert_type(out, 8, context.get_type(source1), 12, result_type);
	convert_type(out, 10, context.get_type(source2), 14, result_type);
	// perform the multiplication
	if(result_type.is_integer()) {
		if(result_type.is_signed()) {
			out << "    mult    $12, $14\n";
		} else {
			out << "    multu   $12, $14\n";
		}
		out << "    nop\n";
		out << "    mflo    $8\n";
		context.store_variable(out, destination, 8);
	} else if(result_type.bytes() == 4) {
		float_operation("mul", out);
		context.store_variable(out, destination, 8);
	} else {
		double_operation("mul", out);
		context.store_variable(out, destination, 8);
	}
}

// *******************************************

DivInstruction::DivInstruction(std::string destination, std::string source1, std::string source2)
: destination(destination), source1(source1), source2(source2) {}

void DivInstruction::Debug(std::ostream &dst) const {
	dst << "    div " << destination << ", " << source1 << ", " << source2 << std::endl;
}

void DivInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	// load the two operands in registers
	context.load_variable(out, source1, 8);
	context.load_variable(out, source2, 10);
	// convert them to the destination type
	Type result_type = context.get_type(destination);
	convert_type(out, 8, context.get_type(source1), 12, result_type);
	convert_type(out, 10, context.get_type(source2), 14, result_type);
	// perform the multiplication
	if(result_type.is_integer()) {
		if(result_type.is_signed()) {
			out << "    div     $12, $14\n";
		} else {
			out << "    divu    $12, $14\n";
		}
		out << "    nop\n";
		out << "    mflo    $8\n";
		context.store_variable(out, destination, 8);
	} else if(result_type.bytes() == 4) {
		float_operation("div", out);
		context.store_variable(out, destination, 8);
	} else {
		double_operation("div", out);
		context.store_variable(out, destination, 8);
	}
}

// *******************************************

ModInstruction::ModInstruction(std::string destination, std::string source1, std::string source2)
: destination(destination), source1(source1), source2(source2) {}

void ModInstruction::Debug(std::ostream &dst) const {
	dst << "    mod " << destination << ", " << source1 << ", " << source2 << std::endl;
}

void ModInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	// load the two operands in registers
	context.load_variable(out, source1, 8);
	context.load_variable(out, source2, 10);
	// convert them to the destination type
	Type result_type = context.get_type(destination);
	convert_type(out, 8, context.get_type(source1), 12, result_type);
	convert_type(out, 10, context.get_type(source2), 14, result_type);
	// perform the multiplication
	if(result_type.is_integer()) {
		if(result_type.is_signed()) {
			out << "    div     $12, $14\n";
		} else {
			out << "    divu    $12, $14\n";
		}
		out << "    nop\n";
		out << "    mfhi    $8\n";
		context.store_variable(out, destination, 8);
	} else {
		throw compile_error("modulo operands must have integral type");
	}
}

// *******************************************

CastInstruction::CastInstruction(std::string destination, std::string source, Type cast_type)
: destination(destination), source(source), cast_type(cast_type) {}

void CastInstruction::Debug(std::ostream &dst) const {
	dst << "    cast " << destination << ", " << source << ", " << cast_type.name() << std::endl;
}

void CastInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	if(context.get_type(source).is_struct() || cast_type.is_struct()) {
		throw compile_error("cannot cast structs");
	}
	context.load_variable(out, source, 8);
	convert_type(out, 8, context.get_type(source), 10, cast_type);
	context.store_variable(out, destination, 10);
}

// *******************************************

FunctionCallInstruction::FunctionCallInstruction(std::string return_result, std::string function_name, std::vector<std::string> arguments)
: return_result(return_result), function_name(function_name), arguments(arguments) {}

void FunctionCallInstruction::Debug(std::ostream &dst) const {
	dst << "    call " << function_name << ", returns " << return_result << std::endl;
	for(std::vector<std::string>::const_iterator itr = arguments.begin(); itr != arguments.end(); ++itr) {
		dst << "      arg " << *itr << std::endl;
	}
}

void FunctionCallInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	// get the information we need about the function
	Type return_type = context.get_type(function_name);
	std::vector<Type> params = context.get_function_parameters(function_name);

	// calculate the total stack to allocate (plus 8 extra bytes just in case)
	unsigned allocate = 8;
	for(std::vector<Type>::const_iterator itr = params.begin(); itr != params.end(); ++itr) {
		align_address(allocate, itr->is_float() ? 8 : 4, 8);
		allocate += itr->bytes();
	}
	if(arguments.size() > params.size()) { // for ellipsis functions
		for(unsigned i = params.size(); i < arguments.size(); ++i) {
			Type arg = context.get_type(arguments.at(i));
			align_address(allocate, arg.is_float() ? 8 : 4, 8);
			allocate += arg.bytes();
		}
	}
	if(return_type.is_struct()) {
		align_address(allocate, 8, 8);
		allocate += return_type.bytes();
	}
	align_address(allocate, 8, 8);
	out << "    addiu   $sp, $sp, -" << allocate << "\n";

	// check that the signatures are compatible
	if(params.size() > arguments.size()) {
		throw compile_error((std::string)"cannot call function '" + function_name + "': incorrect number of parameters.");
	}

	// convert each argument and put it onto the stack
	unsigned current_offset = 0;
	if(return_type.is_struct()) {
		current_offset += 4;
	}
	for(unsigned i = 0; i < params.size(); ++i) {
		Type orig = context.get_type(arguments.at(i));
		Type target = params.at(i);
		align_address(current_offset, target.is_float() ? target.bytes() : 4, 8);
		if(orig.is_struct() || target.is_struct()) {
			if(orig.equals(target)) {
				out << "addiu $3, $sp, " << current_offset << "\n";
				context.copy(out, arguments.at(i), "",orig.bytes());
				current_offset += orig.bytes();
			} else {
				throw compile_error((std::string)"cannot call function '" + function_name + "': incompatible parameters.");
			}
		} else {
			context.load_variable(out, arguments.at(i), 8);
			convert_type(out, 8, orig, 10, target);
			if(target.bytes() == 8) {
				out << "    sw      $10, " << current_offset << "($sp)\n";
				out << "    sw      $11, " << (current_offset+4) << "($sp)\n";
				current_offset += 8;
			} else {
				out << "    sw      $10, " << current_offset << "($sp)\n";
				current_offset += 4;
			}
		}
	}
	// add any additional arguments onto the end of the stack
	for(unsigned i = params.size(); i < arguments.size(); ++i) {
		Type arg = context.get_type(arguments.at(i));
		align_address(current_offset, arg.is_float() ? 8 : 4, 8);
		if(arg.is_struct()) {
			out << "addiu $3, $sp, " << current_offset << "\n";
			context.copy(out, arguments.at(i), "",arg.bytes());
			current_offset += arg.bytes();
		} else {
			context.load_variable(out, arguments.at(i), 10);
			// all floats are promoted to doubles: 6.3.2.2 of the standard
			if(arg.is_float() && arg.bytes() == 4) {
				convert_type(out, 10, arg, 10, Type("double", 0));
			}
			if(arg.bytes() == 8) {
				out << "    sw      $10, " << current_offset << "($sp)\n";
				out << "    sw      $11, " << (current_offset+4) << "($sp)\n";
				current_offset += 8;
			} else {
				out << "    sw      $10, " << current_offset << "($sp)\n";
				current_offset += 4;
			}
		}
	}

	// allocate space for returned struct
	unsigned struct_offset = 0;
	if(return_type.is_struct()) {
		align_address(current_offset, 8, 8);
		struct_offset = current_offset;
		out << "    addiu   $4, $sp, " << struct_offset << "\n";
		out << "    sw      $4, 0($sp)\n";
	}

	// put the first 4 words into registers
	out << "    lw      $4, 0($sp)\n";
	out << "    lw      $5, 4($sp)\n";
	out << "    lw      $6, 8($sp)\n";
	out << "    lw      $7, 12($sp)\n";

	// put floats into their registers if necessary
	if(params.size() > 0 && params.at(0).is_float()) {
		if(params.at(0).bytes() == 4) {
			out << "    lwc1    $f12, 0($sp)\n";
		} else {
			out << "    ldc1    $f12, 0($sp)\n";
		}
		if(params.size() > 1 && params.at(1).is_float()) {
			if(params.at(0).bytes() == 4) {
				out << "    lwc1    $f14, " << params.at(0).bytes() << "($sp)\n";
			} else {
				out << "    ldc1    $f14, " << params.at(0).bytes() << "($sp)\n";
			}
		}
	}

	// jump and link
	out << "    .option	pic0\n";
	out << "    jal     " << function_name << "\n";
	out << "    nop\n";
	out << "    .option	pic2\n";

	// store the result of the function call into our destination
	if(return_type.is_struct()) {
		// copy the struct from its address into the destination
		out << "    addiu   $2, $sp, " << struct_offset << "\n";
		context.copy(out, "", return_result, return_type.bytes());
	} else {
		if(return_type.is_float()) {
			if(return_type.bytes() == 8) {
				out << "    sdc1    $f0, 0($sp)\n";
				out << "    lw      $2, 0($sp)\n";
				out << "    lw      $3, 4($sp)\n";
			} else {
				out << "    mfc1    $2, $f0\n";
			}
		}
		if(return_type.builtin_type != Type::Void)
			context.store_variable(out, return_result, 2);
	}

	// free the stack
	out << "    addiu   $sp, $sp, " << allocate << "\n";
}

// *******************************************

MemberAccessInstruction::MemberAccessInstruction(std::string destination, std::string base, unsigned offset)
: destination(destination), base(base), offset(offset) {}

void MemberAccessInstruction::Debug(std::ostream &dst) const {
	dst << "    member " << destination << ", " << base << " + " << offset << std::endl;
}

void MemberAccessInstruction::PrintMIPS(std::ostream& out, IRContext& context, std::ostream& buff) const {
	context.load_variable(out, base, 8);
	out << "    addiu   $8, $8, " << offset << "\n";
	context.store_variable(out, destination, 8);
}
