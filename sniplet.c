#include <tcl.h>
#include <tclInt.h>
#include <tclIntDecls.h>

#include <string.h>

#ifndef NDEBUG
#define ERR(format,...) do { \
	fprintf(stderr,"%d :%s :",__LINE__, __FUNCTION__); \
	fprintf(stderr,format,##__VA_ARGS__); \
	fputc('\n',stderr); \
}while(0)
#else
#define ERR(format,...) 
#endif
#define GetString(obj) (obj==NULL?"(NIL)":Tcl_GetString(obj))
typedef struct SnipletArg {
	int type;
	Tcl_Obj *name;
	Tcl_Obj *name2;
	Tcl_Obj *value;
} SnipletArg;
#ifndef VAR_SCALAR
#define VAR_SCALAR VAR_LINK
#endif

typedef struct Sniplet {
	struct Tcl_Interp *interp;	
	struct Tcl_Obj *script;	
	struct Tcl_Obj *orig;
	int argc;
	struct SnipletArg *argv;
} Sniplet;


static void Sniplet_FreeInternals(struct Tcl_Obj *obj);
static int Sniplet_SetFromAny(Tcl_Interp *interp, struct Tcl_Obj *obj);
static void Sniplet_DupInternals (struct Tcl_Obj *src,
	struct Tcl_Obj *dst);
static int Sniplet_SetFromAny(Tcl_Interp *interp, struct Tcl_Obj *obj);
static void Sniplet_UpdateString(struct Tcl_Obj *obj);
static int SnipletSaveArgs(Tcl_Interp *interp,Sniplet *snip,Tcl_Obj *arglist);
static int SnipletLoadArgs(Tcl_Interp *interp,Sniplet *snip);

static Tcl_Interp *myInterp=NULL;

static struct Tcl_ObjType SnipletObjType={
	"sniplet",
	Sniplet_FreeInternals,
 	Sniplet_DupInternals,
 	Sniplet_UpdateString,
 	Sniplet_SetFromAny
};

int IsSniplet(Tcl_Obj *obj) {
	ERR("TRACE");
	if (obj==NULL || obj->typePtr==NULL || obj->internalRep.otherValuePtr==NULL)
		return 0;
	if (obj->typePtr==&SnipletObjType || obj->typePtr->name==SnipletObjType.name ||
	    strcmp(obj->typePtr->name,SnipletObjType.name)==0) {
		return 1;
	}
	return 0;
}
Sniplet *SnipletFromObj(struct Tcl_Obj *obj) {
	ERR("TRACE");
	if (obj==NULL)
		return NULL;
	return (Sniplet *)obj->internalRep.otherValuePtr;
}

static void Sniplet_FreeInternals(struct Tcl_Obj *obj) {
	Sniplet *snip;
	ERR("TRACE");
	if (!IsSniplet(obj)) {
    	    return;
	}
	snip=SnipletFromObj(obj);
	if (snip!=NULL) {
		if (snip->argc!=0 && snip->argv!=NULL) {
			int c;
			SnipletArg *arg;
			for(c=0,arg=snip->argv;c<snip->argc;c++,arg++) {
				if (arg->name!=NULL) Tcl_DecrRefCount(arg->name);
				if (arg->name2!=NULL) Tcl_DecrRefCount(arg->name2);
				if (arg->value!=NULL) Tcl_DecrRefCount(arg->value);
			}
			ckfree((VOID *)snip->argv);
		}
		if (snip->script!=NULL)
			Tcl_DecrRefCount(snip->script);
		if (snip->orig!=NULL)
			Tcl_DecrRefCount(snip->orig);
		ckfree(obj->internalRep.otherValuePtr);
		obj->internalRep.otherValuePtr=NULL;
	}
}
static void Sniplet_NewInternals(struct Tcl_Obj *obj) {
	Sniplet *snip;
	ERR("TRACE");
	snip=(Sniplet *)ckalloc(sizeof(Sniplet));
	memset(snip,0,sizeof(Sniplet));
	obj->internalRep.otherValuePtr=snip;
}
Tcl_Obj *TakeObj(Tcl_Obj *obj) {
	ERR("TRACE");
	if (obj==NULL) {
		return NULL;
	}
	if (!Tcl_IsShared(obj))
		obj=Tcl_DuplicateObj(obj);
	Tcl_IncrRefCount(obj);
	return obj;
}
static void Sniplet_DupInternals (struct Tcl_Obj *srcObj,
	struct Tcl_Obj *dstObj) {
	Sniplet *src,*dst;
	ERR("TRACE");
	
	src=SnipletFromObj(srcObj);
	dst=SnipletFromObj(dstObj);	
	if (dst!=NULL) {
		Sniplet_FreeInternals(dstObj);
	}
	Sniplet_NewInternals(dstObj);	
	dst=SnipletFromObj(dstObj);
	
	dst->interp=NULL;
	dst->script=NULL;
	if (src->orig!=NULL) {
		dst->orig=src->orig;
	} else {
		dst->orig=srcObj;
	}
	Tcl_IncrRefCount(dst->orig);
}

static int Sniplet_SetFromAny(Tcl_Interp *interp, struct Tcl_Obj *obj) {
	ERR("TRACE");
	return TCL_ERROR;
}

static void DublicateString(Tcl_Obj *dst,Tcl_Obj *src) {
	ERR("TRACE");
	Tcl_InvalidateStringRep(dst); // delete string presentation
	dst->bytes=ckalloc(src->length);
	memcpy(dst->bytes,
	       src->bytes,
	       src->length);
	dst->length=src->length;
}
static int Sniplet_DirectExecute(Tcl_Obj *obj,Tcl_Obj **optr) {
	Sniplet *snip;
	Tcl_Obj *result;
	Tcl_CallFrame *frame;
	int ret;
	ERR("TRACE");
	if (!IsSniplet(obj))
		return TCL_ERROR;
	snip=SnipletFromObj(obj);
	
	/** hack frame */
	frame=NULL;
	TclPushStackFrame(snip->interp,&frame,NULL,1);
	SnipletLoadArgs(snip->interp,snip);
	
	/** execute script */
	ret=Tcl_EvalObjEx(snip->interp,snip->script,0);
	
	TclPopStackFrame(snip->interp);
	/** unhack frame */
	/** take result */
	result=Tcl_GetObjResult(snip->interp);	
	/** mutate */
	Sniplet_FreeInternals(obj);
	if (result->typePtr==&SnipletObjType) {
		Sniplet_DupInternals(result,obj);
	} else {
		//Sniplet_FreeInternals(obj);
		if (result->typePtr!=NULL && result->typePtr->dupIntRepProc!=NULL) {
			result->typePtr->dupIntRepProc(result,obj);
		} else {
			Tcl_GetString(result);
			
			if (result->bytes!=NULL && result->length!=0) {
				DublicateString(obj,result);
			}
		}
	}
	obj->typePtr=result->typePtr;
	
	if (optr!=NULL)
		(*optr)=result;
	return ret;
}
static int Sniplet_Execute(Tcl_Obj *obj,Tcl_Obj **optr) {
	Sniplet *snip;
	Tcl_Obj *result;
	int ret;
	ERR("TRACE");
	Tcl_Interp *interp;
	snip=SnipletFromObj(obj);
	interp=snip->interp;
	result=NULL;
	if (snip->orig!=NULL) {
		if (IsSniplet(snip->orig))
			ret=Sniplet_DirectExecute(snip->orig,&result);
		
		if (!IsSniplet(snip->orig)) {
			Sniplet_FreeInternals(obj);
			obj->typePtr=result->typePtr;
			if (result->typePtr!=NULL && result->typePtr->dupIntRepProc!=NULL) {
				result->typePtr->dupIntRepProc(result,obj);
			} else {
				Tcl_GetString(result);
				if (result->bytes!=NULL && result->length!=0) {
					DublicateString(obj,result);
				}
			}
		}
	} else {
		ret=Sniplet_DirectExecute(obj,&result);
	}
	Tcl_SetObjResult(interp,result);
	if (optr!=NULL)
		(*optr)=result;
	return ret;
}
static void Sniplet_UpdateString(struct Tcl_Obj *obj) {
	Sniplet *snip;
	Tcl_Obj *result;
	int ret;
	ERR("TRACE");
	if (!IsSniplet(obj)) {
		exit(1);
	}
	snip=SnipletFromObj(obj);
	do {
		result=NULL;
		ret=Sniplet_Execute(obj,&result);
	} while (IsSniplet(obj));
//	Tcl_InvalidateStringRep(obj);
	if (obj->typePtr!=NULL)
		Tcl_GetString(obj);
}
static int ParseNameValue(Tcl_Interp *interp,Tcl_Obj *obj,
	Tcl_Obj **namePtr,
    Tcl_Obj **name2Ptr,
    Tcl_Obj **valuePtr ) {
	Tcl_Obj **objv;
	int objc;
	Tcl_Obj *name=NULL;
	Tcl_Obj *name2=NULL;
	Tcl_Obj *value=NULL;	
	ERR("TRACE");
	ERR("parse %s",GetString(obj));	
	if (Tcl_ListObjGetElements(interp,obj,&objc,&objv)!=TCL_OK) {
		return TCL_ERROR;
	}
	if (objc==0) {
		return TCL_OK;
	}
	/* ToDo : parse hash(key) => name,name2 */
	name=objv[0];
	ERR("name=%s objc=%d",GetString(name),objc);
	if (objc>1) {
		int c;
		value=objv[1];
		for(c=2;c<objc;c++)
			Tcl_ListObjAppendElement(interp,value,objv[c]);
	}
	if (namePtr!=NULL) *namePtr=name;
	if (name2Ptr!=NULL) *name2Ptr=name2;
	if (valuePtr!=NULL) *valuePtr=value;
	return TCL_OK;
}
static int SnipletSaveArgs(Tcl_Interp *interp,Sniplet *snip,Tcl_Obj *arglist) {
	int argc;
	Tcl_Obj **argv;
	int c;
	ERR("TRACE");
	if (arglist==0)
		return TCL_OK;
	if (Tcl_ListObjGetElements(interp,arglist,&argc,&argv)!=TCL_OK) {
		return TCL_ERROR;
	}
	if (argc==0)
		return TCL_OK;
	snip->argv=(SnipletArg *)ckalloc(sizeof(SnipletArg)*argc);
	memset(snip->argv,0,sizeof(SnipletArg)*argc);
	/** сохранить типы,имена и значения переменных в snip->argv */
	for(c=0;c<argc;c++) {
		Var *var;
		Var *array;
		/** переменная может быть переданны как {name value} */
		if (ParseNameValue(interp,argv[c],
			&snip->argv[c].name,
			&snip->argv[c].name2,
		    &snip->argv[c].value)!=TCL_OK) {
			ERR("Cannot parse : %s",GetString(argv[c]));	
			goto ERROR;
		}
		if (snip->argv[c].name!=NULL) snip->argv[c].name=TakeObj(snip->argv[c].name);
		if (snip->argv[c].name2!=NULL) snip->argv[c].name2=TakeObj(snip->argv[c].name2);
		if (snip->argv[c].value!=NULL) snip->argv[c].value=TakeObj(snip->argv[c].value);
		var=TclObjLookupVar(interp,snip->argv[c].name,NULL,
		    0,NULL,0,0,&array);
		if (var==NULL) {
			/** возможно там args */
			if (c==argc-1 && snip->argv[c].name2 == NULL) {
				Tcl_GetString(snip->argv[c].name);
				if (strcmp(snip->argv[c].name->bytes,"args")==0) {
					/* действительно args */
					snip->argv[c].type|=VAR_IS_ARGS;
					continue;
				}
			}
			/** ToDo: переделать */
			snip->argv[c].type|=VAR_SCALAR;
			continue;
			ERR("Lookupfailed for %s",GetString(snip->argv[c].name));
			goto ERROR;
		}
		if (TclIsVarScalar(var)) {
			snip->argv[c].type|=VAR_SCALAR;
			if (snip->argv[c].value==NULL) {
				snip->argv[c].value=TakeObj(var->value.objPtr);
			}
		} else if (TclIsVarArray(var)) {
			snip->argv[c].type|=VAR_ARRAY;
			if (snip->argv[c].value==NULL) {
				Tcl_Obj *cmd[4];
				cmd[0]=Tcl_NewStringObj("array",-1);
				cmd[1]=Tcl_NewStringObj("get",-1);
				ERR("name=%s",GetString(snip->argv[c].name));
				cmd[2]=snip->argv[c].name;
				if (TclInvokeStringCommand(Tcl_GetCommandFromObj(interp,cmd[0]),interp,3,cmd)!=TCL_OK) {
					exit(1);
				}
				snip->argv[c].value=TakeObj(Tcl_GetObjResult(interp));
				
			}
		} else {
			ERR("Unsupported type for var");
			goto ERROR;
		}
	}
	snip->argc=argc;
	return TCL_OK;
ERROR:
	ERR("BLYA!!!!!!!!!!!!!!!!!!!!!!!!!");
	argc=c;
	for(c=0;c<=argc;c++) {
		if (snip->argv[c].name!=NULL) Tcl_DecrRefCount(snip->argv[c].name);
		if (snip->argv[c].name2!=NULL) Tcl_DecrRefCount(snip->argv[c].name2);
		if (snip->argv[c].value!=NULL) Tcl_DecrRefCount(snip->argv[c].value);
	}
	ckfree((VOID *)snip->argv);
	snip->argv=NULL;
	return TCL_ERROR;
}
static int SnipletLoadArgs(Tcl_Interp *interp,Sniplet *snip) {
	int c;
	SnipletArg *arg;
	ERR("TRACE");
	if (snip==NULL || snip->argc==0)
		return TCL_OK;
	for(c=0,arg=snip->argv;c<snip->argc;c++,arg++) {
		if (arg->type&VAR_IS_ARGS) {
			Tcl_ObjSetVar2(interp,arg->name,NULL,NULL,VAR_ARGUMENT);
		} else if (arg->type&VAR_ARRAY) {
			Tcl_Obj *cmd[4];
			cmd[0]=Tcl_NewStringObj("array",-1);
			cmd[1]=Tcl_NewStringObj("set",-1);
			cmd[2]=arg->name;
			cmd[3]=arg->value;
			if (TclInvokeStringCommand(Tcl_GetCommandFromObj(interp,cmd[0]),interp,4,cmd)!=TCL_OK) {
				exit(1);
			}
		} else if (arg->type==VAR_SCALAR) {
			Tcl_ObjSetVar2(interp,arg->name,NULL,arg->value,VAR_ARGUMENT);
		} else {
			return TCL_ERROR;
		}
	}
	return TCL_OK;
}
static Tcl_Obj *CreateSniplet(Tcl_Interp *interp,
	Tcl_Obj *nameListObj,
    Tcl_Obj *script,
    int lazy) {
	Tcl_Obj *obj;
	Sniplet *snip;
	ERR("TRACE");
	obj=Tcl_NewObj();
	obj->typePtr=&SnipletObjType;
	Sniplet_NewInternals(obj);
	snip=SnipletFromObj(obj);
	snip->interp=interp;
	snip->script=TakeObj(script);
	SnipletSaveArgs(interp,snip,nameListObj);
	Tcl_InvalidateStringRep(obj);
	if (lazy==0) {
		Tcl_GetString(obj);
		return obj;
	} else {
		return obj;
	}
}

static int Sniplet_Cmd(ClientData cdata, 
	Tcl_Interp *interp, 
	int objc, 
	Tcl_Obj *const objv[]) {
	struct Tcl_Obj *obj;
	struct Tcl_Obj *names;
	struct Tcl_Obj *script;
	ERR("TRACE");
	if (objc==2) {
		names=NULL;
		script=objv[1];
	} else if (objc==3) {
		names=objv[1];
		script=objv[2];
	} else {
		Tcl_WrongNumArgs(interp,objc,objv,"Should be ?{names} {script}");
		return TCL_ERROR;
	}
	obj=CreateSniplet(interp,names,script,0);
	Tcl_SetObjResult(interp,obj);
	if (obj==NULL)
		return TCL_ERROR;
	return TCL_OK;
}
static int Sniplet_LazyCmd(ClientData cdata, 
	Tcl_Interp *interp, 
	int objc, 
	Tcl_Obj *const objv[]) {
	struct Tcl_Obj *obj;
	struct Tcl_Obj *names;
	struct Tcl_Obj *script;
	ERR("TRACE");
	if (objc==2) {
		names=NULL;
		script=objv[1];
	} else if (objc==3) {
		names=objv[1];
		script=objv[2];
	} else {
		Tcl_WrongNumArgs(interp,objc,objv,"Should be ?{names} {script}");
		return TCL_ERROR;
	}
	obj=CreateSniplet(interp,names,script,1);
	Tcl_SetObjResult(interp,obj);
	if (obj==NULL)
		return TCL_ERROR;
	return TCL_OK;
}
                          
int 
Sniplet_Init(Tcl_Interp *interp) {
	ERR("TRACE");
	myInterp=interp;
	if (Tcl_InitStubs(interp,TCL_VERSION,0) == NULL) {
		return TCL_ERROR;
	}
	/* changed this to check for an error - GPS */
	if (Tcl_PkgProvide(interp, "sniplet", "1.0") == TCL_ERROR) {
		return TCL_ERROR;
	}
	Tcl_RegisterObjType(&SnipletObjType);
	Tcl_CreateObjCommand(interp, "sniplet", Sniplet_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(interp, "lazy", Sniplet_LazyCmd, NULL, NULL);
   	return TCL_OK;
}
