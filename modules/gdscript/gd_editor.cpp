/*************************************************************************/
/*  gd_editor.cpp                                                        */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                    http://www.godotengine.org                         */
/*************************************************************************/
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                 */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/
#include "gd_script.h"
#include "gd_compiler.h"


void GDScriptLanguage::get_comment_delimiters(List<String> *p_delimiters) const {

	p_delimiters->push_back("#");
	p_delimiters->push_back("\"\"\"");

}
void GDScriptLanguage::get_string_delimiters(List<String> *p_delimiters) const {

	p_delimiters->push_back("\" \"");
	p_delimiters->push_back("' '");


}
String GDScriptLanguage::get_template(const String& p_class_name, const String& p_base_class_name) const {

	String _template = String()+
	"\nextends %BASE%\n\n"+
	"# member variables here, example:\n"+
	"# var a=2\n"+
	"# var b=\"textvar\"\n\n"+
	"func _ready():\n"+
	"\t# Initalization here\n"+
	"\tpass\n"+
	"\n"+
	"\n";

	return _template.replace("%BASE%",p_base_class_name);
}




bool GDScriptLanguage::validate(const String& p_script, int &r_line_error,int &r_col_error,String& r_test_error, const String& p_path,List<String> *r_functions) const {

	GDParser parser;

	Error err = parser.parse(p_script,p_path.get_base_dir(),true,p_path);
	if (err) {
		r_line_error=parser.get_error_line();
		r_col_error=parser.get_error_column();
		r_test_error=parser.get_error();
		return false;
	} else {

		const GDParser::Node *root = parser.get_parse_tree();
		ERR_FAIL_COND_V(root->type!=GDParser::Node::TYPE_CLASS,false);

		const GDParser::ClassNode *cl = static_cast<const GDParser::ClassNode*>(root);
		Map<int,String> funcs;
		for(int i=0;i<cl->functions.size();i++) {

			funcs[cl->functions[i]->line]=cl->functions[i]->name;
		}

		for(int i=0;i<cl->static_functions.size();i++) {

			funcs[cl->static_functions[i]->line]=cl->static_functions[i]->name;
		}

		for (Map<int,String>::Element *E=funcs.front();E;E=E->next()) {

			r_functions->push_back(E->get()+":"+itos(E->key()));
		}


	}

	return true;
}

bool GDScriptLanguage::has_named_classes() const {

	return false;
}

int GDScriptLanguage::find_function(const String& p_function,const String& p_code) const {

	GDTokenizerText tokenizer;
	tokenizer.set_code(p_code);
	int indent=0;
	while(tokenizer.get_token()!=GDTokenizer::TK_EOF && tokenizer.get_token()!=GDTokenizer::TK_ERROR) {

		if (tokenizer.get_token()==GDTokenizer::TK_NEWLINE) {
			indent=tokenizer.get_token_line_indent();
		}
		//print_line("TOKEN: "+String(GDTokenizer::get_token_name(tokenizer.get_token())));
		if (indent==0 && tokenizer.get_token()==GDTokenizer::TK_PR_FUNCTION && tokenizer.get_token(1)==GDTokenizer::TK_IDENTIFIER) {

			String identifier = tokenizer.get_token_identifier(1);
			if (identifier==p_function) {
				return tokenizer.get_token_line();
			}
		}
		tokenizer.advance();
		//print_line("NEXT: "+String(GDTokenizer::get_token_name(tokenizer.get_token())));

	}
	return -1;
}

Script *GDScriptLanguage::create_script() const {

	return memnew( GDScript );
}

/* DEBUGGER FUNCTIONS */


bool GDScriptLanguage::debug_break_parse(const String& p_file, int p_line,const String& p_error) {
	//break because of parse error

    if (ScriptDebugger::get_singleton() && Thread::get_caller_ID()==Thread::get_main_ID()) {

	_debug_parse_err_line=p_line;
	_debug_parse_err_file=p_file;
	_debug_error=p_error;
	ScriptDebugger::get_singleton()->debug(this,false);
	return true;
    } else {
	return false;
    }

}

bool GDScriptLanguage::debug_break(const String& p_error,bool p_allow_continue) {

    if (ScriptDebugger::get_singleton() && Thread::get_caller_ID()==Thread::get_main_ID()) {

	_debug_parse_err_line=-1;
	_debug_parse_err_file="";
	_debug_error=p_error;
	ScriptDebugger::get_singleton()->debug(this,p_allow_continue);
	return true;
    } else {
	return false;
    }

}

String GDScriptLanguage::debug_get_error() const {

    return _debug_error;
}

int GDScriptLanguage::debug_get_stack_level_count() const {

	if (_debug_parse_err_line>=0)
		return 1;


	return _debug_call_stack_pos;
}
int GDScriptLanguage::debug_get_stack_level_line(int p_level) const {

	if (_debug_parse_err_line>=0)
		return _debug_parse_err_line;

    ERR_FAIL_INDEX_V(p_level,_debug_call_stack_pos,-1);

    int l = _debug_call_stack_pos - p_level -1;

    return *(_call_stack[l].line);

}
String GDScriptLanguage::debug_get_stack_level_function(int p_level) const {

	if (_debug_parse_err_line>=0)
		return "";

    ERR_FAIL_INDEX_V(p_level,_debug_call_stack_pos,"");
    int l = _debug_call_stack_pos - p_level -1;
    return _call_stack[l].function->get_name();
}
String GDScriptLanguage::debug_get_stack_level_source(int p_level) const {

	if (_debug_parse_err_line>=0)
		return _debug_parse_err_file;

    ERR_FAIL_INDEX_V(p_level,_debug_call_stack_pos,"");
    int l = _debug_call_stack_pos - p_level -1;
    return _call_stack[l].function->get_script()->get_path();

}
void GDScriptLanguage::debug_get_stack_level_locals(int p_level,List<String> *p_locals, List<Variant> *p_values, int p_max_subitems,int p_max_depth) {

	if (_debug_parse_err_line>=0)
		return;

    ERR_FAIL_INDEX(p_level,_debug_call_stack_pos);
    int l = _debug_call_stack_pos - p_level -1;

    GDFunction *f = _call_stack[l].function;

    List<Pair<StringName,int> > locals;

    f->debug_get_stack_member_state(*_call_stack[l].line,&locals);
    for( List<Pair<StringName,int> >::Element *E = locals.front();E;E=E->next() ) {

	p_locals->push_back(E->get().first);
	p_values->push_back(_call_stack[l].stack[E->get().second]);
    }

}
void GDScriptLanguage::debug_get_stack_level_members(int p_level,List<String> *p_members, List<Variant> *p_values, int p_max_subitems,int p_max_depth) {

	if (_debug_parse_err_line>=0)
		return;

    ERR_FAIL_INDEX(p_level,_debug_call_stack_pos);
    int l = _debug_call_stack_pos - p_level -1;


    GDInstance *instance = _call_stack[l].instance;

    if (!instance)
	return;

    Ref<GDScript> script = instance->get_script();
    ERR_FAIL_COND( script.is_null() );


    const Map<StringName,GDScript::MemberInfo>& mi = script->debug_get_member_indices();

    for(const Map<StringName,GDScript::MemberInfo>::Element *E=mi.front();E;E=E->next()) {

	p_members->push_back(E->key());
	p_values->push_back( instance->debug_get_member_by_index(E->get().index));
    }

}
void GDScriptLanguage::debug_get_globals(List<String> *p_locals, List<Variant> *p_values, int p_max_subitems,int p_max_depth) {

    //no globals are really reachable in gdscript
}
String GDScriptLanguage::debug_parse_stack_level_expression(int p_level,const String& p_expression,int p_max_subitems,int p_max_depth) {

	if (_debug_parse_err_line>=0)
		return "";
	return "";
}

void GDScriptLanguage::get_recognized_extensions(List<String> *p_extensions) const {

	p_extensions->push_back("gd");
}


void GDScriptLanguage::get_public_functions(List<MethodInfo> *p_functions) const {


	for(int i=0;i<GDFunctions::FUNC_MAX;i++) {

		p_functions->push_back(GDFunctions::get_info(GDFunctions::Function(i)));
	}
}

void GDScriptLanguage::get_public_constants(List<Pair<String,Variant> > *p_constants) const {

	Pair<String,Variant> pi;
	pi.first="PI";
	pi.second=Math_PI;
	p_constants->push_back(pi);
}

String GDScriptLanguage::make_function(const String& p_class,const String& p_name,const StringArray& p_args) const {

	String s="func "+p_name+"(";
	if (p_args.size()) {
		s+=" ";
		for(int i=0;i<p_args.size();i++) {
			if (i>0)
				s+=", ";
			s+=p_args[i];
		}
		s+=" ";
	}
	s+="):\n\tpass # replace with function body\n";

	return s;

}

#if defined(DEBUG_METHODS_ENABLED) && defined(TOOLS_ENABLED)

struct GDCompletionIdentifier {

	StringName obj_type;
	Variant::Type type;
	Variant value; //im case there is a value, also return it
};



static GDCompletionIdentifier _get_type_from_variant(const Variant& p_variant) {

	GDCompletionIdentifier t;
	t.type=p_variant.get_type();
	t.value=p_variant;
	if (p_variant.get_type()==Variant::OBJECT) {
		Object *obj = p_variant;
		if (obj) {
			//if (obj->cast_to<GDNativeClass>()) {
			//	t.obj_type=obj->cast_to<GDNativeClass>()->get_name();
			//	t.value=Variant();
			//} else {
				t.obj_type=obj->get_type();
			//}
		}
	}
	return t;
}

static GDCompletionIdentifier _get_type_from_pinfo(const PropertyInfo& p_info) {

	GDCompletionIdentifier t;
	t.type=p_info.type;
	if (p_info.hint==PROPERTY_HINT_RESOURCE_TYPE) {
		t.obj_type=p_info.hint_string;
	}
	return t;
}

struct GDCompletionContext {

	const GDParser::ClassNode *_class;
	const GDParser::FunctionNode *function;
	const GDParser::BlockNode *block;
	Object* base;
	String base_path;

};


static Ref<Reference> _get_parent_class(GDCompletionContext& context) {



	if (context._class->extends_used) {
		//do inheritance
		String path = context._class->extends_file;

		Ref<GDScript> script;
		Ref<GDNativeClass> native;

		if (path!="") {
			//path (and optionally subclasses)

			if (path.is_rel_path()) {

				path=context.base_path.plus_file(path);
			}
			script = ResourceLoader::load(path);
			if (script.is_null()) {
				return REF();
			}
			if (script->is_valid()) {

				return REF();
			}
			//print_line("EXTENDS PATH: "+path+" script is "+itos(script.is_valid())+" indices is "+itos(script->member_indices.size())+" valid? "+itos(script->valid));

			if (context._class->extends_class.size()) {

				for(int i=0;i<context._class->extends_class.size();i++) {

					String sub = context._class->extends_class[i];
					if (script->get_subclasses().has(sub)) {

						script=script->get_subclasses()[sub];
					} else {

						return REF();
					}
				}
			}

			if (script.is_valid())
				return script;

		} else {

			if (context._class->extends_class.size()==0) {
				ERR_PRINT("BUG");
				return REF();
			}

			String base=context._class->extends_class[0];
			const GDParser::ClassNode *p = context._class->owner;
			Ref<GDScript> base_class;
#if 0
			while(p) {

				if (p->subclasses.has(base)) {

					base_class=p->subclasses[base];
					break;
				}
				p=p->_owner;
			}
#endif
			if (base_class.is_valid()) {
#if 0
				for(int i=1;i<context._class->extends_class.size();i++) {

					String subclass=context._class->extends_class[i];

					if (base_class->subclasses.has(subclass)) {

						base_class=base_class->subclasses[subclass];
					} else {

						print_line("Could not find subclass: "+subclass);
						return _get_type_from_class(context); //fail please
					}
				}

				script=base_class;
#endif

			} else {

				if (context._class->extends_class.size()>1) {

					return REF();


				}
				//if not found, try engine classes
				if (!GDScriptLanguage::get_singleton()->get_global_map().has(base)) {

					return REF();
				}

				int base_idx = GDScriptLanguage::get_singleton()->get_global_map()[base];
				native = GDScriptLanguage::get_singleton()->get_global_array()[base_idx];
				if (!native.is_valid()) {

					print_line("Global not a class: '"+base+"'");

				}
				return native;
			}


		}

	}

	return Ref<Reference>();
}


static GDCompletionIdentifier _get_native_class(GDCompletionContext& context) {

	//eeh...
	GDCompletionIdentifier id;
	id.type=Variant::NIL;

	REF pc  = _get_parent_class(context);
	if (!pc.is_valid()) {
		return id;
	}
	Ref<GDNativeClass> nc = pc;
	Ref<GDScript> s = pc;

	if (s.is_null() && nc.is_null()) {
		return id;
	}
	while(!s.is_null()) {
		nc=s->get_native();
		s=s->get_base();
	}
	if (nc.is_null()) {
		return id;
	}



	id.type=Variant::OBJECT;
	if (context.base)
		id.value=context.base;
	id.obj_type=nc->get_name();
	return id;
}

static bool _guess_identifier_type(GDCompletionContext& context,int p_line,const StringName& p_identifier,GDCompletionIdentifier &r_type);


static bool _guess_expression_type(GDCompletionContext& context,const GDParser::Node* p_node,int p_line,GDCompletionIdentifier &r_type) {


	if (p_node->type==GDParser::Node::TYPE_CONSTANT) {

		const GDParser::ConstantNode *cn=static_cast<const GDParser::ConstantNode *>(p_node);

		r_type=_get_type_from_variant(cn->value);

		return true;
	} else if (p_node->type==GDParser::Node::TYPE_DICTIONARY) {

		r_type.type=Variant::DICTIONARY;


		//what the heck, fill it anyway
		const GDParser::DictionaryNode *an = static_cast<const GDParser::DictionaryNode *>(p_node);
		Dictionary d;
		for(int i=0;i<an->elements.size();i++) {
			GDCompletionIdentifier k;
			if (_guess_expression_type(context,an->elements[i].key,p_line,k) && k.value.get_type()!=Variant::NIL) {
				GDCompletionIdentifier v;
				if (_guess_expression_type(context,an->elements[i].value,p_line,v)) {
					d[k.value]=v.value;
				}

			}
		}
		r_type.value=d;
		return true;
	} else if (p_node->type==GDParser::Node::TYPE_ARRAY) {

		r_type.type=Variant::ARRAY;
		//what the heck, fill it anyway
		const GDParser::ArrayNode *an = static_cast<const GDParser::ArrayNode *>(p_node);
		Array arr;
		arr.resize(an->elements.size());
		for(int i=0;i<an->elements.size();i++) {
			GDCompletionIdentifier ci;
			if (_guess_expression_type(context,an->elements[i],p_line,ci)) {
				arr[i]=ci.value;
			}
		}
		r_type.value=arr;
		return true;

	} else if (p_node->type==GDParser::Node::TYPE_BUILT_IN_FUNCTION) {

		MethodInfo mi = GDFunctions::get_info(static_cast<const GDParser::BuiltInFunctionNode*>(p_node)->function);
		r_type=_get_type_from_pinfo(mi.return_val);

		return true;
	} else if (p_node->type==GDParser::Node::TYPE_IDENTIFIER) {

		return _guess_identifier_type(context,p_line-1,static_cast<const GDParser::IdentifierNode *>(p_node)->name,r_type);
	} else if (p_node->type==GDParser::Node::TYPE_SELF) {
		//eeh...

		r_type=_get_native_class(context);
		return r_type.type!=Variant::NIL;

	} else if (p_node->type==GDParser::Node::TYPE_OPERATOR) {


		const GDParser::OperatorNode *op = static_cast<const GDParser::OperatorNode *>(p_node);
		if (op->op==GDParser::OperatorNode::OP_CALL) {
			if (op->arguments[0]->type==GDParser::Node::TYPE_TYPE) {

				const GDParser::TypeNode *tn = static_cast<const GDParser::TypeNode *>(op->arguments[0]);
				r_type.type=tn->vtype;
				return true;
			} else if (op->arguments[0]->type==GDParser::Node::TYPE_BUILT_IN_FUNCTION) {


				const GDParser::BuiltInFunctionNode *bin = static_cast<const GDParser::BuiltInFunctionNode *>(op->arguments[0]);
				return _guess_expression_type(context,bin,p_line,r_type);

			} else if (op->arguments.size()>1 && op->arguments[1]->type==GDParser::Node::TYPE_IDENTIFIER) {


				GDCompletionIdentifier base;
				if (!_guess_expression_type(context,op->arguments[0],p_line,base))
					return false;

				StringName id = static_cast<const GDParser::IdentifierNode *>(op->arguments[1])->name;

				if (base.type==Variant::OBJECT) {

					if (id.operator String()=="new" && base.value.get_type()==Variant::OBJECT) {
						Object *obj = base.value;
						if (obj && obj->cast_to<GDNativeClass>()) {
							GDNativeClass *gdnc = obj->cast_to<GDNativeClass>();
							r_type.type=Variant::OBJECT;
							r_type.value=Variant();
							r_type.obj_type=gdnc->get_name();
							return true;
						}
					}

					if (ObjectTypeDB::has_method(base.obj_type,id)) {

#ifdef TOOLS_ENABLED
						MethodBind *mb = ObjectTypeDB::get_method(base.obj_type,id);
						PropertyInfo pi = mb->get_argument_info(-1);

						//try calling the function if constant and all args are constant, should not crash..
						Object *baseptr = base.value;

						if (baseptr && mb->is_const() && pi.type==Variant::OBJECT) {
							bool all_valid=true;
							Vector<Variant> args;
							for(int i=2;i<op->arguments.size();i++) {
								GDCompletionIdentifier arg;

								if (_guess_expression_type(context,op->arguments[i],p_line,arg)) {
									if (arg.value.get_type()!=Variant::NIL && arg.value.get_type()!=Variant::OBJECT) { // calling with object seems dangerous, i don' t know
										args.push_back(arg.value);
									} else {
										all_valid=false;
										break;
									}
								} else {
									all_valid=false;
								}
							}
							if (all_valid) {
								Vector<const Variant*> argptr;
								for(int i=0;i<args.size();i++) {
									argptr.push_back(&args[i]);
								}

								Variant::CallError ce;
								Variant ret=mb->call(baseptr,argptr.ptr(),argptr.size(),ce);


								if (ce.error==Variant::CallError::CALL_OK && ret.get_type()!=Variant::NIL) {

									if (ret.get_type()!=Variant::OBJECT || ret.operator Object*()!=NULL) {

										r_type=_get_type_from_variant(ret);
										return true;
									}
								}

							}
						}

						r_type.type=pi.type;
						if (pi.hint==PROPERTY_HINT_RESOURCE_TYPE) {
							r_type.obj_type=pi.hint_string;
						}



						return true;
#else
						return false;
#endif
					} else {
						return false;
					}
				} else {
					//method for some variant..
					Variant::CallError ce;
					Variant v = Variant::construct(base.type,NULL,0,ce);
					List<MethodInfo> mi;
					v.get_method_list(&mi);
					for (List<MethodInfo>::Element *E=mi.front();E;E=E->next()) {

						if (!E->get().name.begins_with("_") && E->get().name==id.operator String()) {


							MethodInfo mi = E->get();
							r_type.type=mi.return_val.type;
							if (mi.return_val.hint==PROPERTY_HINT_RESOURCE_TYPE) {
								r_type.obj_type=mi.return_val.hint_string;
							}
							return true;
						}
					}

				}


			}
		} else if (op->op==GDParser::OperatorNode::OP_INDEX || op->op==GDParser::OperatorNode::OP_INDEX_NAMED) {

			GDCompletionIdentifier p1;
			GDCompletionIdentifier p2;



			if (op->op==GDParser::OperatorNode::OP_INDEX_NAMED) {

				if (op->arguments[1]->type==GDParser::Node::TYPE_IDENTIFIER) {
					String id = static_cast<const GDParser::IdentifierNode*>(op->arguments[1])->name;
					p2.type=Variant::STRING;
					p2.value=id;
				}

			} else {
				if (op->arguments[1]) {
					if (!_guess_expression_type(context,op->arguments[1],p_line,p2)) {

						return false;
					}
				}
			}

			if (op->arguments[0]->type==GDParser::Node::TYPE_ARRAY) {

				const GDParser::ArrayNode *an = static_cast<const GDParser::ArrayNode *>(op->arguments[0]);
				if (p2.value.is_num()) {
					int index = p2.value;
					if (index<0 || index>=an->elements.size())
						return false;
					return _guess_expression_type(context,an->elements[index],p_line,r_type);
				}

			} else if (op->arguments[0]->type==GDParser::Node::TYPE_DICTIONARY) {

				const GDParser::DictionaryNode *dn = static_cast<const GDParser::DictionaryNode *>(op->arguments[0]);

				if (p2.value.get_type()==Variant::NIL)
					return false;

				for(int i=0;i<dn->elements.size();i++) {

					GDCompletionIdentifier k;

					if (!_guess_expression_type(context,dn->elements[i].key,p_line,k)) {

						return false;
					}

					if (k.value.get_type()==Variant::NIL)
						return false;

					if (k.value==p2.value) {

						return _guess_expression_type(context,dn->elements[i].value,p_line,r_type);
					}
				}

			} else {

				if (op->arguments[0]) {
					if (!_guess_expression_type(context,op->arguments[0],p_line,p1)) {

						return false;
					}

				}

				if (p1.value.get_type()==Variant::OBJECT) {
					//??
				} else if (p1.value.get_type()!=Variant::NIL) {

					bool valid;
					Variant ret = p1.value.get(p2.value,&valid);
					if (valid) {
						r_type=_get_type_from_variant(ret);
						return true;
					}

				} else {
					if (p1.type!=Variant::NIL) {
						Variant::CallError ce;
						Variant base = Variant::construct(p1.type,NULL,0,ce);
						bool valid;
						Variant ret = base.get(p2.value,&valid);
						if (valid) {
							r_type=_get_type_from_variant(ret);
							return true;
						}
					}
				}
			}

		} else {


			Variant::Operator vop = Variant::OP_MAX;
			switch(op->op) {
				case GDParser::OperatorNode::OP_ADD: vop=Variant::OP_ADD; break;
				case GDParser::OperatorNode::OP_SUB: vop=Variant::OP_SUBSTRACT; break;
				case GDParser::OperatorNode::OP_MUL: vop=Variant::OP_MULTIPLY; break;
				case GDParser::OperatorNode::OP_DIV: vop=Variant::OP_DIVIDE; break;
				case GDParser::OperatorNode::OP_MOD: vop=Variant::OP_MODULE; break;
				case GDParser::OperatorNode::OP_SHIFT_LEFT: vop=Variant::OP_SHIFT_LEFT; break;
				case GDParser::OperatorNode::OP_SHIFT_RIGHT: vop=Variant::OP_SHIFT_RIGHT; break;
				case GDParser::OperatorNode::OP_BIT_AND: vop=Variant::OP_BIT_AND; break;
				case GDParser::OperatorNode::OP_BIT_OR: vop=Variant::OP_BIT_OR; break;
				case GDParser::OperatorNode::OP_BIT_XOR: vop=Variant::OP_BIT_XOR; break;
				default:{}

			}



			if (vop==Variant::OP_MAX)
				return false;



			GDCompletionIdentifier p1;
			GDCompletionIdentifier p2;

			if (op->arguments[0]) {
				if (!_guess_expression_type(context,op->arguments[0],p_line,p1)) {

					return false;
				}

			}

			if (op->arguments.size()>1) {
				if (!_guess_expression_type(context,op->arguments[1],p_line,p2)) {

					return false;
				}
			}

			Variant::CallError ce;
			bool v1_use_value = p1.value.get_type()!=Variant::NIL && p1.value.get_type()!=Variant::OBJECT;
			Variant v1 = (v1_use_value)?p1.value:Variant::construct(p1.type,NULL,0,ce);
			bool v2_use_value = p2.value.get_type()!=Variant::NIL && p2.value.get_type()!=Variant::OBJECT;
			Variant v2 = (v2_use_value)?p2.value:Variant::construct(p2.type,NULL,0,ce);
			// avoid potential invalid ops
			if ((vop==Variant::OP_DIVIDE || vop==Variant::OP_MODULE) && v2.get_type()==Variant::INT) {
				v2=1;
				v2_use_value=false;
			}
			if (vop==Variant::OP_DIVIDE && v2.get_type()==Variant::REAL) {
				v2=1.0;
				v2_use_value=false;
			}

			Variant r;
			bool valid;
			Variant::evaluate(vop,v1,v2,r,valid);
			if (!valid)
				return false;
			r_type.type=r.get_type();
			if (v1_use_value && v2_use_value)
				r_type.value=r;

			return true;

		}

	}

	return false;
}

static bool _guess_identifier_type_in_block(GDCompletionContext& context,int p_line,const StringName& p_identifier,GDCompletionIdentifier &r_type) {



	const GDParser::Node *last_assign=NULL;
	int last_assign_line=-1;

	for (int i=0;i<context.block->statements.size();i++) {

		if (context.block->statements[i]->line>p_line)
			continue;


		if (context.block->statements[i]->type==GDParser::BlockNode::TYPE_LOCAL_VAR) {

			const GDParser::LocalVarNode *lv=static_cast<const GDParser::LocalVarNode *>(context.block->statements[i]);

			if (lv->assign && lv->name==p_identifier) {

				last_assign=lv->assign;
				last_assign_line=context.block->statements[i]->line;
			}
		}

		if (context.block->statements[i]->type==GDParser::BlockNode::TYPE_OPERATOR) {
			const GDParser::OperatorNode *op = static_cast<const GDParser::OperatorNode *>(context.block->statements[i]);
			if (op->op==GDParser::OperatorNode::OP_ASSIGN) {

				if (op->arguments.size() && op->arguments[0]->type==GDParser::Node::TYPE_IDENTIFIER) {

					const GDParser::IdentifierNode *id = static_cast<const GDParser::IdentifierNode *>(op->arguments[0]);

					if (id->name==p_identifier) {

						last_assign=op->arguments[1];
						last_assign_line=context.block->statements[i]->line;
					}
				}
			}
		}
	}

	//use the last assignment, (then backwards?)
	if (last_assign) {

		return _guess_expression_type(context,last_assign,last_assign_line,r_type);
	}


	return false;
}

static bool _guess_identifier_type(GDCompletionContext& context,int p_line,const StringName& p_identifier,GDCompletionIdentifier &r_type) {

	//go to block first


	const GDParser::BlockNode *block=context.block;

	while(block) {

		GDCompletionContext c = context;
		c.block=block;

		if (_guess_identifier_type_in_block(c,p_line,p_identifier,r_type)) {
			return true;
		}

		block=block->parent_block;
	}

	//guess from argument if virtual
	if (context.function && context.function->name!=StringName()) {

		int argindex = -1;

		for(int i=0;i<context.function->arguments.size();i++) {

			if (context.function->arguments[i]==p_identifier) {
				argindex=i;
				break;
			}

		}

		if (argindex!=-1) {
			GDCompletionIdentifier id =_get_native_class(context);
			if (id.type==Variant::OBJECT && id.obj_type!=StringName()) {
				//this kinda sucks but meh

				List<MethodInfo> vmethods;
				ObjectTypeDB::get_virtual_methods(id.obj_type,&vmethods);
				for (List<MethodInfo>::Element *E=vmethods.front();E;E=E->next()) {


					if (E->get().name==context.function->name && argindex<E->get().arguments.size()) {

						PropertyInfo arg=E->get().arguments[argindex];

						int scp = arg.name.find(":");
						if (scp!=-1) {


							r_type.type=Variant::OBJECT;
							r_type.obj_type=arg.name.substr(scp+1,arg.name.length());
							return true;

						} else {

							r_type.type=arg.type;
							if (arg.hint==PROPERTY_HINT_RESOURCE_TYPE)
								r_type.obj_type=arg.hint_string;
							return true;
						}
					}
				}
			}
		}
	}

	//guess type in constant

	for(int i=0;i<context._class->constant_expressions.size();i++) {

		if (context._class->constant_expressions[i].identifier==p_identifier) {

			ERR_FAIL_COND_V( context._class->constant_expressions[i].expression->type!=GDParser::Node::TYPE_CONSTANT, false );
			r_type=_get_type_from_variant(static_cast<const GDParser::ConstantNode*>(context._class->constant_expressions[i].expression)->value );
			return true;
		}
	}

	if (!(context.function && context.function->_static)) {

		for(int i=0;i<context._class->variables.size();i++) {

			if (context._class->variables[i].identifier==p_identifier) {

				if (context._class->variables[i]._export.type!=Variant::NIL) {

					r_type=_get_type_from_pinfo(context._class->variables[i]._export);
					return true;
				} else if (context._class->variables[i].expression) {
					return _guess_expression_type(context,context._class->variables[i].expression,context._class->variables[i].line,r_type);
				}
			}
		}
	}



	for(Map<StringName,int>::Element *E=GDScriptLanguage::get_singleton()->get_global_map().front();E;E=E->next()) {
		if (E->key()==p_identifier) {

			r_type=_get_type_from_variant(GDScriptLanguage::get_singleton()->get_global_array()[E->get()]);
			return true;
		}

	}
	return false;
}


static void _find_identifiers_in_block(GDCompletionContext& context,int p_line,bool p_only_functions,Set<String>& result) {

	if (p_only_functions)
		return;

	for (int i=0;i<context.block->statements.size();i++) {

		if (context.block->statements[i]->line>p_line)
			continue;


		if (context.block->statements[i]->type==GDParser::BlockNode::TYPE_LOCAL_VAR) {

			const GDParser::LocalVarNode *lv=static_cast<const GDParser::LocalVarNode *>(context.block->statements[i]);
			result.insert(lv->name.operator String());
		}
	}
}

static void _find_identifiers_in_class(GDCompletionContext& context,bool p_static,bool p_only_functions,Set<String>& result) {

	if (!p_static && !p_only_functions) {

		for(int i=0;i<context._class->variables.size();i++) {
			result.insert(context._class->variables[i].identifier);
		}
	}
	if (!p_only_functions) {

		for(int i=0;i<context._class->constant_expressions.size();i++) {
			result.insert(context._class->constant_expressions[i].identifier);
		}

		for(int i=0;i<context._class->subclasses.size();i++) {
			result.insert(context._class->subclasses[i]->name);
		}

	}

	for(int i=0;i<context._class->static_functions.size();i++) {
		if (context._class->static_functions[i]->arguments.size())
			result.insert(context._class->static_functions[i]->name.operator String()+"(");
		else
			result.insert(context._class->static_functions[i]->name.operator String()+"()");
	}

	if (!p_static) {

		for(int i=0;i<context._class->functions.size();i++) {
			if (context._class->functions[i]->arguments.size())
				result.insert(context._class->functions[i]->name.operator String()+"(");
			else
				result.insert(context._class->functions[i]->name.operator String()+"()");
		}
	}

	//globals

	Ref<Reference> base = _get_parent_class(context);

	while(true) {

		Ref<GDScript> script = base;
		Ref<GDNativeClass> nc = base;
		if (script.is_valid()) {

			if (!p_static && !p_only_functions) {
				for (const Set<StringName>::Element *E=script->get_members().front();E;E=E->next()) {
					result.insert(E->get().operator String());
				}
			}

			if (!p_only_functions) {
				for (const Map<StringName,Variant>::Element *E=script->get_constants().front();E;E=E->next()) {
					result.insert(E->key().operator String());
				}
			}

			for (const Map<StringName,GDFunction>::Element *E=script->get_member_functions().front();E;E=E->next()) {
				if (!p_static || E->get().is_static()) {
					if (E->get().get_argument_count())
						result.insert(E->key().operator String()+"(");
					else
						result.insert(E->key().operator String()+"()");
				}
			}

			if (!p_only_functions)	{
				for (const Map<StringName,Ref<GDScript> >::Element *E=script->get_subclasses().front();E;E=E->next()) {
					result.insert(E->key().operator String());
				}
			}

			base=script->get_base();
			if (base.is_null())
				base=script->get_native();
		} else if (nc.is_valid()) {

			if (!p_only_functions) {

				StringName type = nc->get_name();
				List<String> constants;
				ObjectTypeDB::get_integer_constant_list(type,&constants);
				for(List<String>::Element *E=constants.front();E;E=E->next()) {
					result.insert(E->get());
				}

				List<MethodInfo> methods;
				ObjectTypeDB::get_method_list(type,&methods);
				for(List<MethodInfo>::Element *E=methods.front();E;E=E->next()) {
					if (E->get().name.begins_with("_"))
						continue;
					if (E->get().arguments.size())
						result.insert(E->get().name+"(");
					else
						result.insert(E->get().name+"()");
				}
			}
			break;
		} else
			break;

	}

}

static void _find_identifiers(GDCompletionContext& context,int p_line,bool p_only_functions,Set<String>& result) {

	const GDParser::BlockNode *block=context.block;

	while(block) {

		GDCompletionContext c = context;
		c.block=block;

		_find_identifiers_in_block(c,p_line,p_only_functions,result);
		block=block->parent_block;
	}

	const GDParser::ClassNode *clss=context._class;

	bool _static=context.function && context.function->_static;

	while(clss) {
		GDCompletionContext c = context;
		c._class=clss;
		c.block=NULL;
		c.function=NULL;
		_find_identifiers_in_class(c,_static,p_only_functions,result);
		clss=clss->owner;
	}

	for(int i=0;i<GDFunctions::FUNC_MAX;i++) {

		result.insert(GDFunctions::get_func_name(GDFunctions::Function(i)));
	}

	static const char*_type_names[Variant::VARIANT_MAX]={
		"null","bool","int","float","String","Vector2","Rect2","Vector3","Matrix32","Plane","Quat","AABB","Matrix3","Trasnform",
		"Color","Image","NodePath","RID","Object","InputEvent","Dictionary","Array","RawArray","IntArray","FloatArray","StringArray",
		"Vector2Array","Vector3Array","ColorArray"};

	for(int i=0;i<Variant::VARIANT_MAX;i++) {
		result.insert(_type_names[i]);
	}

	for(const Map<StringName,int>::Element *E=GDScriptLanguage::get_singleton()->get_global_map().front();E;E=E->next()) {
		result.insert(E->key().operator String());
	}
}


static String _get_visual_datatype(const PropertyInfo& p_info,bool p_isarg=true) {

	String n = p_info.name;
	int idx = n.find(":");
	if (idx!=-1) {
		return n.substr(idx+1,n.length());
	}

	if (p_info.type==Variant::OBJECT && p_info.hint==PROPERTY_HINT_RESOURCE_TYPE)
		return p_info.hint_string;
	if (p_info.type==Variant::NIL) {
		if (p_isarg)
			return "var";
		else
			return "void";
	}

	return Variant::get_type_name(p_info.type);
}

static void _make_function_hint(const GDParser::FunctionNode* p_func,int p_argidx,String& arghint) {

	arghint="func "+p_func->name+"(";
	for (int i=0;i<p_func->arguments.size();i++) {
		if (i>0)
			arghint+=", ";
		else
			arghint+=" ";

		if (i==p_argidx) {
			arghint+=String::chr(0xFFFF);
		}
		arghint+=p_func->arguments[i].operator String();
		int deffrom = p_func->arguments.size()-p_func->default_values.size();

		if (i>=deffrom) {
			int defidx = deffrom-i;

			if (defidx>=0 && defidx<p_func->default_values.size()) {

				if (p_func->default_values[defidx]->type==GDParser::Node::TYPE_OPERATOR) {

					const GDParser::OperatorNode *op=static_cast<const GDParser::OperatorNode *>(p_func->default_values[defidx]);
					if (op->op==GDParser::OperatorNode::OP_ASSIGN) {
						const GDParser::ConstantNode *cn=static_cast<const GDParser::ConstantNode *>(op->arguments[1]);
						arghint+="="+cn->value.get_construct_string();

					}
				} else {

				}
			}
		}

		if (i==p_argidx) {
			arghint+=String::chr(0xFFFF);
		}
	}
	if (p_func->arguments.size()>0)
		arghint+=" ";
	arghint+=")";
}


static void _find_type_arguments(const GDParser::Node*p_node,int p_line,const StringName& p_method,const GDCompletionIdentifier& id, int p_argidx, Set<String>& result, String& arghint) {


	if (id.type==Variant::OBJECT && id.obj_type!=StringName()) {


		MethodBind *m = ObjectTypeDB::get_method(id.obj_type,p_method);
		if (!m)
			return;

		if (p_method.operator String()=="connect") {


			if (p_argidx==0) {
				List<MethodInfo> sigs;
				ObjectTypeDB::get_signal_list(id.obj_type,&sigs);
				for (List<MethodInfo>::Element *E=sigs.front();E;E=E->next()) {
					result.insert("\""+E->get().name+"\"");
				}
			}
			/*if (p_argidx==2) {

				ERR_FAIL_COND(p_node->type!=GDParser::Node::TYPE_OPERATOR);
				const GDParser::OperatorNode *op=static_cast<const GDParser::OperatorNode *>(p_node);
				if (op->arguments.size()>)

			}*/
		} else {

			Object *obj=id.value;
			if (obj) {
				List<String> options;
				obj->get_argument_options(p_method,p_argidx,&options);
				for(List<String>::Element *E=options.front();E;E=E->next()) {

					result.insert(E->get());
				}
			}

		}

		arghint = _get_visual_datatype(m->get_argument_info(-1),false)+" "+p_method.operator String()+String("(");

		for(int i=0;i<m->get_argument_count();i++) {
			if (i>0)
				arghint+=", ";
			else
				arghint+=" ";

			if (i==p_argidx) {
				arghint+=String::chr(0xFFFF);
			}
			String n = m->get_argument_info(i).name;
			int dp = n.find(":");
			if (dp!=-1)
				n=n.substr(0,dp);
			arghint+=_get_visual_datatype(m->get_argument_info(i))+" "+n;
			int deffrom = m->get_argument_count()-m->get_default_argument_count();


			if (i>=deffrom) {
				int defidx = i-deffrom;

				if (defidx>=0 && defidx<m->get_default_argument_count()) {
					Variant v= m->get_default_argument(i);
					arghint+="="+v.get_construct_string();
				}
			}

			if (i==p_argidx) {
				arghint+=String::chr(0xFFFF);
			}

		}
		if (m->get_argument_count()>0)
			arghint+=" ";


		arghint+=")";

	}
}


static void _find_call_arguments(GDCompletionContext& context,const GDParser::Node* p_node, int p_line,int p_argidx, Set<String>& result, String& arghint) {



	if (!p_node || p_node->type!=GDParser::Node::TYPE_OPERATOR) {

		return;
	}

	const GDParser::OperatorNode *op = static_cast<const GDParser::OperatorNode *>(p_node);

	if (op->op!=GDParser::OperatorNode::OP_CALL) {

		return;
	}

	if (op->arguments[0]->type==GDParser::Node::TYPE_BUILT_IN_FUNCTION) {
		//complete built-in function
		const GDParser::BuiltInFunctionNode *fn = static_cast<const GDParser::BuiltInFunctionNode*>(op->arguments[0]);
		MethodInfo mi = GDFunctions::get_info(fn->function);

		arghint = _get_visual_datatype(mi.return_val,false)+" "+GDFunctions::get_func_name(fn->function)+String("(");
		for(int i=0;i<mi.arguments.size();i++) {
			if (i>0)
				arghint+=", ";
			else
				arghint+=" ";
			if (i==p_argidx) {
				arghint+=String::chr(0xFFFF);
			}
			arghint+=_get_visual_datatype(mi.arguments[i])+" "+mi.arguments[i].name;
			if (i==p_argidx) {
				arghint+=String::chr(0xFFFF);
			}

		}
		if (mi.arguments.size()>0)
			arghint+=" ";
		arghint+=")";

	} else if (op->arguments[0]->type==GDParser::Node::TYPE_TYPE) {
		//complete built-in function
		const GDParser::TypeNode *tn = static_cast<const GDParser::TypeNode*>(op->arguments[0]);

		List<MethodInfo> mil;
		Variant::get_constructor_list(tn->vtype,&mil);

		for(List<MethodInfo>::Element *E=mil.front();E;E=E->next()) {

			MethodInfo mi = E->get();
			if (mi.arguments.size()==0)
				continue;
			if (E->prev())
				arghint+="\n";
			arghint += Variant::get_type_name(tn->vtype)+" "+Variant::get_type_name(tn->vtype)+String("(");
			for(int i=0;i<mi.arguments.size();i++) {
				if (i>0)
					arghint+=", ";
				else
					arghint+=" ";
				if (i==p_argidx) {
					arghint+=String::chr(0xFFFF);
				}
				arghint+=_get_visual_datatype(mi.arguments[i])+" "+mi.arguments[i].name;
				if (i==p_argidx) {
					arghint+=String::chr(0xFFFF);
				}

			}
			if (mi.arguments.size()>0)
				arghint+=" ";
			arghint+=")";
		}

	} else if (op->arguments.size()>=2 && op->arguments[1]->type==GDParser::Node::TYPE_IDENTIFIER) {
		//make sure identifier exists...

		const GDParser::IdentifierNode *id=static_cast<const GDParser::IdentifierNode *>(op->arguments[1]);

		if (op->arguments[0]->type==GDParser::Node::TYPE_SELF) {
			//self, look up

			for(int i=0;i<context._class->static_functions.size();i++) {
				if (context._class->static_functions[i]->name==id->name) {
					_make_function_hint(context._class->static_functions[i],p_argidx,arghint);
					return;
				}
			}

			if (context.function && !context.function->_static) {

				for(int i=0;i<context._class->functions.size();i++) {
					if (context._class->functions[i]->name==id->name) {
						_make_function_hint(context._class->functions[i],p_argidx,arghint);
						return;
					}
				}
			}

			Ref<Reference> base = _get_parent_class(context);

			while(true) {

				Ref<GDScript> script = base;
				Ref<GDNativeClass> nc = base;
				if (script.is_valid()) {


					for (const Map<StringName,GDFunction>::Element *E=script->get_member_functions().front();E;E=E->next()) {

						if (E->key()==id->name) {

							if (context.function && context.function->_static && !E->get().is_static())
								continue;


							arghint = "func "+id->name.operator String()+String("(");
							for(int i=0;i<E->get().get_argument_count();i++) {
								if (i>0)
									arghint+=", ";
								else
									arghint+=" ";
								if (i==p_argidx) {
									arghint+=String::chr(0xFFFF);
								}
								arghint+=E->get().get_argument_name(i);
								int deffrom = E->get().get_argument_count()-E->get().get_default_argument_count();
								if (i>=deffrom) {
									int defidx = deffrom-i;
									if (defidx>=0 && defidx<E->get().get_default_argument_count()) {
										arghint+="="+E->get().get_default_argument(defidx).get_construct_string();
									}
								}
								if (i==p_argidx) {
									arghint+=String::chr(0xFFFF);
								}

							}
							if (E->get().get_argument_count()>0)
								arghint+=" ";
							arghint+=")";
							return;
						}
					}

					base=script->get_base();
					if (base.is_null())
						base=script->get_native();
				} else if (nc.is_valid()) {

					if (context.function && !context.function->_static) {

						GDCompletionIdentifier ci;
						ci.type=Variant::OBJECT;
						ci.obj_type=nc->get_name();
						if (!context._class->owner)
							ci.value=context.base;

						_find_type_arguments(p_node,p_line,id->name,ci,p_argidx,result,arghint);
						//guess type..
						/*
						List<MethodInfo> methods;
						ObjectTypeDB::get_method_list(type,&methods);
						for(List<MethodInfo>::Element *E=methods.front();E;E=E->next()) {
							//if (E->get().arguments.size())
							//	result.insert(E->get().name+"(");
							//else
							//	result.insert(E->get().name+"()");
						}*/
					}
					break;
				} else
					break;

			}
		} else {


			GDCompletionIdentifier ci;
			if (_guess_expression_type(context,op->arguments[0],p_line,ci)) {

				_find_type_arguments(p_node,p_line,id->name,ci,p_argidx,result,arghint);
				return;
			}

		}

	}
#if 0
	bool _static=context.function->_static;




	for(int i=0;i<context._class->static_functions.size();i++) {
		if (context._class->static_functions[i]->arguments.size())
			result.insert(context._class->static_functions[i]->name.operator String()+"(");
		else
			result.insert(context._class->static_functions[i]->name.operator String()+"()");
	}

	if (!p_static) {

		for(int i=0;i<context._class->functions.size();i++) {
			if (context._class->functions[i]->arguments.size())
				result.insert(context._class->functions[i]->name.operator String()+"(");
			else
				result.insert(context._class->functions[i]->name.operator String()+"()");
		}
	}

	Ref<Reference> base = _get_parent_class(context);

	while(true) {

		Ref<GDScript> script = base;
		Ref<GDNativeClass> nc = base;
		if (script.is_valid()) {

			if (!p_static && !p_only_functions) {
				for (const Set<StringName>::Element *E=script->get_members().front();E;E=E->next()) {
					result.insert(E->get().operator String());
				}
			}

			if (!p_only_functions) {
				for (const Map<StringName,Variant>::Element *E=script->get_constants().front();E;E=E->next()) {
					result.insert(E->key().operator String());
				}
			}

			for (const Map<StringName,GDFunction>::Element *E=script->get_member_functions().front();E;E=E->next()) {
				if (!p_static || E->get().is_static()) {
					if (E->get().get_argument_count())
						result.insert(E->key().operator String()+"(");
					else
						result.insert(E->key().operator String()+"()");
				}
			}

			if (!p_only_functions)	{
				for (const Map<StringName,Ref<GDScript> >::Element *E=script->get_subclasses().front();E;E=E->next()) {
					result.insert(E->key().operator String());
				}
			}

			base=script->get_base();
			if (base.is_null())
				base=script->get_native();
		} else if (nc.is_valid()) {

			if (!p_only_functions) {

				StringName type = nc->get_name();
				List<String> constants;
				ObjectTypeDB::get_integer_constant_list(type,&constants);
				for(List<String>::Element *E=constants.front();E;E=E->next()) {
					result.insert(E->get());
				}

				List<MethodInfo> methods;
				ObjectTypeDB::get_method_list(type,&methods);
				for(List<MethodInfo>::Element *E=methods.front();E;E=E->next()) {
					if (E->get().arguments.size())
						result.insert(E->get().name+"(");
					else
						result.insert(E->get().name+"()");
				}
			}
			break;
		} else
			break;

	}

	for(int i=0;i<GDFunctions::FUNC_MAX;i++) {

		result.insert(GDFunctions::get_func_name(GDFunctions::Function(i)));
	}

#endif

}

Error GDScriptLanguage::complete_code(const String& p_code, const String& p_base_path, Object*p_owner, List<String>* r_options, String &r_call_hint) {
	//print_line( p_code.replace(String::chr(0xFFFF),"<cursor>"));

	GDParser p;
	Error err = p.parse(p_code,p_base_path);
	bool isfunction=false;
	Set<String> options;

	GDCompletionContext context;
	context._class=p.get_completion_class();
	context.block=p.get_completion_block();
	context.function=p.get_completion_function();
	context.base=p_owner;
	context.base_path=p_base_path;

	switch(p.get_completion_type()) {

		case GDParser::COMPLETION_NONE: {
			print_line("No completion");
		} break;
		case GDParser::COMPLETION_BUILT_IN_TYPE_CONSTANT: {
			print_line("Built in type constant");
			List<StringName> constants;
			Variant::get_numeric_constants_for_type(p.get_completion_built_in_constant(),&constants);
			for(List<StringName>::Element *E=constants.front();E;E=E->next()) {
				options.insert(E->get().operator String());
			}


		} break;
		case GDParser::COMPLETION_FUNCTION:
			isfunction=true;
		case GDParser::COMPLETION_IDENTIFIER: {

			_find_identifiers(context,p.get_completion_line(),isfunction,options);
		} break;
		case GDParser::COMPLETION_PARENT_FUNCTION: {
			print_line("parent function");

		} break;
		case GDParser::COMPLETION_METHOD:
			isfunction=true;
		case GDParser::COMPLETION_INDEX: {

			const GDParser::Node *node = p.get_completion_node();
			if (node->type!=GDParser::Node::TYPE_OPERATOR)
				break;




			GDCompletionIdentifier t;
			if (_guess_expression_type(context,static_cast<const GDParser::OperatorNode *>(node)->arguments[0],p.get_completion_line(),t)) {

				if (t.type==Variant::OBJECT && t.obj_type!=StringName()) {


					if (t.value.get_type()) {
						Object *obj=t.value;
						if (obj) {
							GDScript *scr = obj->cast_to<GDScript>();
							while (scr) {

								if (!isfunction) {
									for (const Map<StringName,Variant>::Element *E=scr->get_constants().front();E;E=E->next()) {
										options.insert(E->key());
									}
								}
								for (const Map<StringName,GDFunction>::Element *E=scr->get_member_functions().front();E;E=E->next()) {
									options.insert(E->key());
								}

								if (scr->get_base().is_valid())
									scr=scr->get_base().ptr();
								else
									scr=NULL;
							}
						}
					}


					if (!isfunction) {
						ObjectTypeDB::get_integer_constant_list(t.obj_type,r_options);
					}
					List<MethodInfo> mi;
					ObjectTypeDB::get_method_list(t.obj_type,&mi);
					for (List<MethodInfo>::Element *E=mi.front();E;E=E->next()) {

						if (E->get().name.begins_with("_"))
							continue;

						if (E->get().arguments.size())
							options.insert(E->get().name+"(");
						else
							options.insert(E->get().name+"()");

					}
				} else {


					if (t.type==Variant::INPUT_EVENT) {

						//this is hardcoded otherwise it's not obvious
						Set<String> exclude;

						for(int i=0;i<InputEvent::TYPE_MAX;i++) {

							InputEvent ie;
							ie.type=InputEvent::Type(i);
							static const char*evnames[]={
								"# Common",
								"# Key",
								"# MouseMotion",
								"# MouseButton",
								"# JoyMotion",
								"# JoyButton",
								"# ScreenTouch",
								"# ScreenDrag",
								"# Action"
							};

							r_options->push_back(evnames[i]);

							Variant v = ie;

							if (i==0) {
								List<MethodInfo> mi;
								v.get_method_list(&mi);
								for (List<MethodInfo>::Element *E=mi.front();E;E=E->next()) {
									r_options->push_back(E->get().name+"(");

								}

							}

							List<PropertyInfo> pi;
							v.get_property_list(&pi);

							for (List<PropertyInfo>::Element *E=pi.front();E;E=E->next()) {

								if (i==0)
									exclude.insert(E->get().name);
								else if (exclude.has(E->get().name))
									continue;

								r_options->push_back(E->get().name);
							}
						}
						return OK;
					} else {
						if (t.value.get_type()==Variant::NIL) {
							Variant::CallError ce;
							t.value=Variant::construct(t.type,NULL,0,ce);
						}


						if (!isfunction) {
							List<PropertyInfo> pl;
							t.value.get_property_list(&pl);
							for (List<PropertyInfo>::Element *E=pl.front();E;E=E->next()) {

								if (E->get().name.find("/")==-1)
									options.insert(E->get().name);
							}
						}

						List<MethodInfo> mi;
						t.value.get_method_list(&mi);
						for (List<MethodInfo>::Element *E=mi.front();E;E=E->next()) {
							if (E->get().arguments.size())
								options.insert(E->get().name+"(");
							else
								options.insert(E->get().name+"()");

						}
					}
				}
			}


		} break;
		case GDParser::COMPLETION_CALL_ARGUMENTS: {

			_find_call_arguments(context,p.get_completion_node(),p.get_completion_line(),p.get_completion_argument_index(),options,r_call_hint);
		} break;


	}


	for(Set<String>::Element *E=options.front();E;E=E->next()) {
		r_options->push_back(E->get());
	}

	return OK;
}

#else

Error GDScriptLanguage::complete_code(const String& p_code, const String& p_base_path, Object*p_owner, List<String>* r_options, String &r_call_hint) {
	return OK;
}

#endif


void GDScriptLanguage::auto_indent_code(String& p_code,int p_from_line,int p_to_line) const {


	Vector<String> lines = p_code.split("\n");
	List<int> indent_stack;

	for(int i=0;i<lines.size();i++) {

		String l = lines[i];
		int tc=0;
		for(int j=0;j<l.length();j++) {
			if (l[j]==' ' || l[j]=='\t') {

				tc++;
			} else {
				break;
			}
		}


		String st = l.substr(tc,l.length()).strip_edges();
		if (st=="" || st.begins_with("#"))
			continue; //ignore!

		int ilevel=0;
		if (indent_stack.size()) {
			ilevel=indent_stack.back()->get();
		}

		if (tc>ilevel) {
			indent_stack.push_back(tc);
		} else if (tc<ilevel) {
			while(indent_stack.size() && indent_stack.back()->get()>tc) {
				indent_stack.pop_back();
			}

			if (indent_stack.size() && indent_stack.back()->get()!=tc)
				indent_stack.push_back(tc); //this is not right but gets the job done
		}

		if (i>=p_from_line) {

			l="";
			for(int j=0;j<indent_stack.size();j++)
				l+="\t";
			l+=st;


		} else if (i>p_to_line) {
			break;
		}

		//print_line(itos(indent_stack.size())+","+itos(tc)+": "+l);
		lines[i]=l;
	}

	p_code="";
	for(int i=0;i<lines.size();i++) {
		if (i>0)
			p_code+="\n";
		p_code+=lines[i];
	}

}
