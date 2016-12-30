#include "core.h"
#include "resources.h"
#include "structure.h"
#include "commands.h"
#include "player.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

/* World map sketch 
	10111000 
	11101100 
	00111000 
	11101000
*/

static void game_interface(){

};

static bool game_refreshProgress(){
	return TRUE;
	err_exit;
};

static bool chamber_setup(WORLD *w){
	if (NULL != w && NULL != w->allchambers){
		CHAMBER *aux;
		//Here's some hardcoded number, which express exacly the world map sketch above, when in binary notation.
		uint ms = 3102488808, mask = (1 << (sizeof(uint) * BITS_IN_BYTE - 1));

		#ifdef DEBUG
			printf("D: will create chamber list following this model:\n");
			for(uint Q = (1 << (sizeof(uint)*BITS_IN_BYTE - 1)), counter = 0; Q > 0; Q >>= 1, ++counter){
				printf("%d", (ms & Q) >= 1);
				if ((counter + 1) % GLOBALV_MAPW == 0){
					printf("\n");
					Q >>= 2;
				};
			};
		#endif

		for (byte i = 0; i < (GLOBALV_MAPH * GLOBALV_MAPW); ++i, mask >>= 1){
			if (ms & mask){
				aux = chinit();
				if (aux != NULL){
					++(w->nused);
					*(w->allchambers + i) = aux;
				} else {
					//Something went wrong, abort.
					for(byte k = i; k > 0; --k)
						if (*(w->allchambers + k) != NULL){
							free(*(w->allchambers + k));
							--(w->nused);
						};
					printf("E: \"%s\" failed.\n", __FUNCTION__);
					return FALSE;
				};
			} else *(w->allchambers + i) = NULL;
			if ((i + 1) % GLOBALV_MAPW == 0)
				mask >>= 2;
		};

		#ifdef DEBUG
			printf("D: successfully created chamber list:\n");
			for(byte Q = 0; Q < (GLOBALV_MAPH * GLOBALV_MAPW); ++Q){
				printf("%d", *(w->allchambers + Q) != NULL);
				if (((Q + 1) % GLOBALV_MAPW) == 0)
					printf("\n");
			};
		#endif
		return TRUE;
	};
	err_exit;
};

static bool game_setup(GAME *const g){
	if (g != NULL){
		#ifdef DEBUG
			printf("D: trying to start WORLD structure...\n");
		#endif
		g->world = winit();

		if (g->world != NULL){
			#ifdef DEBUG
				printf("D: success, now will try to start PLAYER structure...\n");
			#endif
			g->player = pinit();

			if (g->player != NULL){
				#ifdef DEBUG
					printf("D: success, now will try to start COMMAND structure...\n");
				#endif
				g->command = cinit();

				if (g->command != NULL){
					#ifdef DEBUG
						printf("D: success on main structure initialization...\n");
					#endif
					return TRUE;
				} else {
					#ifdef DEBUG
						printf("D: failed to start COMMAND structure, aborting...\n");
					#endif
				};
				pdestroy(&g->player);
			} else {
				#ifdef DEBUG
					printf("D: failed to start PLAYER structure, aborting...\n");
				#endif
			};
			wdestroy(&g->world);
		} else {
			#ifdef DEBUG
				printf("D:failed to start WORLD structure, aborting...\n");
			#endif
		};
	};
	err_exit;
};

static bool ch_adjch_setup(CHAMBER *ch, byte num, ...){
	if (NULL != ch && num > 0){

		#ifdef DEBUG
			if(ch->string == NULL)
				printf("D: will start path construction on [%p] chamber...\n", ch);
			else
				printf("D: will start path construction on \"%s\" chamber...\n", ch->string);
		#endif

		CHAMBER *aux;
		PATH *p;
		ch->adjchambers = realloc(ch->adjchambers, sizeof(PATH *) * (ch->adjnum + num));
		if (NULL != ch->adjchambers){
			va_list adjchambers;
			va_start(adjchambers, num);
			for (byte i = num; i > 0; --i){
				aux = va_arg(adjchambers, CHAMBER *);
				if (aux != NULL){
					p = malloc(sizeof(PATH));
					if (p != NULL){
						//Set the new path
						*(ch->adjchambers + ch->adjnum + i - 1) = p;
						p->a = ch;
						p->b = aux;
						p->open = TRUE;
						p->string = NULL;
						//Set the new path on the b chamber
						p->b->adjchambers = realloc(p->b->adjchambers, sizeof(PATH *) * (p->b->adjnum + 1));
						if (p->b->adjchambers != NULL){
							*(p->b->adjchambers + p->b->adjnum++) = p;

							#ifdef DEBUG
								if (p->a->string != NULL && p->b->string != NULL)
									printf("D: successfully set path between {\"%s\"<->\"%s\"} chambers...\n", p->a->string, p->b->string);
								else
									printf("D: successfully set path between {[%p]<->[%p]} chambers...\n", p->a, p->b);
							#endif
						};
					} else {
						//Something went wrong, backtrack everything and abort.
						for (byte j = (i - 1); j < num; ++j)
							free(*(ch->adjchambers + ch->adjnum + j));
						if (ch->adjnum > 0)
							ch->adjchambers = realloc(ch->adjchambers, sizeof(PATH *) * ch->adjnum);
						else {
							free(ch->adjchambers);
							ch->adjchambers = NULL;
						}
						va_end(adjchambers);
						printf("E: \"%s\" failed.\n", __FUNCTION__);
						return FALSE;
					};
				};
			};
			#ifdef DEBUG
				if (ch->string != NULL)
					printf("D: endded path construction on \"%s\" chamber...\n", ch->string);
				else
					printf("D: endded path construction on [%p] chamber...\n", ch);
			#endif
			ch->adjnum += num;
			va_end(adjchambers);
			return TRUE;
		};
	};
	err_exit;
};

static bool ch_iatcv_setup(CHAMBER *ch, byte num, ...){
	if (NULL != ch && num > 0){
		ch->iactives = malloc(sizeof(IACTV *) * num);
		if (NULL != ch->iactives){
			IACTV *aux = NULL;
			va_list iactv;
			va_start(iactv, num);
			for (byte i = num; i > 0; --i){
				aux = va_arg(iactv, IACTV *);
				if (aux != NULL){
					*(ch->iactives + i - 1) = aux;
					++ch->actnum;
					aux = NULL;
				} else printf("W: invalid argument in \"%s\".\n", __FUNCTION__);
			};
			va_end(iactv);

			#ifdef DEBUG
				printf("D: put (%hu) interactives on \"%s\" chamber.\n", ch->actnum, ch->string);
			#endif

			return TRUE;
		}
	};
	err_exit;
};

/* World map sketch 
	101P1000 
	P1P0P100 
	001P1000 
	P1P0P000
*/

static void *wload(void *vw){
	WORLD *w = (WORLD *) vw;
	bool *ret = malloc(sizeof(bool));
	*ret = FALSE;

	#ifdef DEBUG
		printf("D: will load hardcoded world map...\n");
	#endif
	if (w != NULL && w->allchambers != NULL){
		byte counter = 0;
		//Hardcoded section
		//First line
		counter += (*(w->allchambers + 3))->adjch_setup(*(w->allchambers + 3), 2, *(w->allchambers + 2), *(w->allchambers + 4));
		//Second line
		counter += (*(w->allchambers + 6))->adjch_setup(*(w->allchambers + 6), 2, *(w->allchambers + 0), *(w->allchambers + 7));
		counter += (*(w->allchambers + 8))->adjch_setup(*(w->allchambers + 8), 3, *(w->allchambers + 7), 
			*(w->allchambers + 2), *(w->allchambers + 14));
		counter += (*(w->allchambers + 10))->adjch_setup(*(w->allchambers + 10), 3, *(w->allchambers + 4), 
			*(w->allchambers + 11), *(w->allchambers + 16));
		//Third line
		counter += (*(w->allchambers + 15))->adjch_setup(*(w->allchambers + 15), 2, *(w->allchambers + 14), *(w->allchambers + 16));
		//Last line
		counter += (*(w->allchambers + 18))->adjch_setup(*(w->allchambers + 18), 1, *(w->allchambers + 19));
		counter += (*(w->allchambers + 20))->adjch_setup(*(w->allchambers + 19), 2, *(w->allchambers + 7), *(w->allchambers + 14));
		counter += (*(w->allchambers + 22))->adjch_setup(*(w->allchambers + 22), 1, *(w->allchambers + 16));
		#ifdef DEBUG
			printf("D: created (%hhu/%u) path between chambers.\n", counter, GLOBAV_NUMPATHS);
		#endif
		if (counter == GLOBAV_NUMPATHS)
			*ret = TRUE;
	};
	return ret;
};

static void *wgetlabels(void *vw){
	WORLD *w = (WORLD *) vw;
	bool *ret = malloc(sizeof(bool));
	*ret = FALSE;

	#ifdef DEBUG
		printf("D: started process of chamber labelling...\n");
	#endif
	if (w != NULL && w->allchambers != NULL){
		FILE *fp = fopen("./data/chlabels", "r");
		if (fp != NULL){
			CHAMBER *aux = NULL;
			byte index = 0, counter = 0;
			char *string;

			for(byte i = 0; i < w->nused; aux = *(w->allchambers + index++), ++i){
				string = get_string(fp);
				if (string != NULL){
					++counter;
					while(aux == NULL)
						aux = *(w->allchambers + index++);
					for(size_t i = 0; *(string + i) != '\0'; decodify((byte *) (string + i)), ++i);
					aux->string = string;
					#ifdef DEBUG
						printf("D: got new chamber label: \"%s\" at chamber index [%d].\n", aux->string, index - 1);
					#endif
				} else printf("E: something went wrong in chamber labelling (can't get label).\n");
			};
			#ifdef DEBUG
				printf("D: loaded (%hhu/%hhu) chamber labels.\n", counter, w->nused);
			#endif

			fclose(fp);
			*ret = TRUE;
		} else printf("E: can't access \"./data/chlabels\".\n");
	};
	return ret;
};

static IACTV *iload(IACTV *i, char const path[]){
	if (access(path, R_OK) == 0)
		#ifdef DEBUG
			printf("D: file \"%s\" is found and is readable.\n", path);
		#endif
	else {
		printf("E: can't access \"%s\" file.\n", path);
		free(i);
		return NULL;
	};

	if (i != NULL && access(path, R_OK) == 0){
		FILE *fp = fopen(path, "r");
		char *aux = NULL;
		byte n, security_v = 255;
		if (fp != NULL){
			i->label = get_string(fp);
			if (i->label != NULL){
				for(n = 0; *(i->label + n) != '\0'; decodify((byte *) (i->label + n)), ++n);
				if (!feof(fp)){
					while(security_v > 0 && (aux == NULL || *aux != '#')){
						aux = get_string(fp);
						if (aux != NULL && *aux != '#'){
							for (n = 0; *(aux + n) == '\0'; decodify((byte *) (aux + n)), ++n);
							i->script = realloc(i->script, sizeof(char *) * (i->scpnum + 1));
							*(i->script + i->scpnum++) = aux;
						};
						--security_v;
					};
					if (aux != NULL && *aux == '#'){
						free(aux);
						aux = NULL;
						if (!feof(fp)){
							while(security_v > 0 && (aux == NULL || *aux != '#')){
								aux = get_string(fp);
								if (aux != NULL && *aux != '#'){
									for (n = 0; *(aux + n) == '\0'; decodify((byte *) (aux + n)), ++n);
									i->actions = realloc(i->actions, sizeof(char *) * (i->actnum + 1));
									*(i->actions + i->actnum++) = aux;
								};
								--security_v;
							};
							if (aux != NULL && *aux == '#')
								free(aux);
						};
					};

					if (security_v == 0)
						printf("W: security_v ran out! (possible non-stopping loop error due to bad extern file)\n");

					#ifdef DEBUG
						if (i->label != NULL)
							printf("D: successfully loaded \"%s\" file on \"%s\" IACTV:\n", path, i->label);
						else
							printf("D: successfully loaded \"%s\" file on [%p] (NIL label) IACTV:\n", path, i);
						printf("ACTIONS (%hu):\n", i->actnum);
						for (n = i->actnum; n > 0; printf("\"%s\"\n", *(i->actions + n - 1)), --n);
						printf("\nSCRIPT (%hu):\n", i->scpnum);
						for (n = i->scpnum; n > 0; printf("\"%s\"\n", *(i->script + n - 1)), --n);
					#endif
				};
			} else printf("E: can't get label on [%p] structure.\n", i);
			fclose(fp);
		} else printf("E: can't open \"%s\" file.\n", path);
	};
	return i;
};

static void *isetup(void *vw){
	WORLD *w = (WORLD *) vw;
	bool *ret = malloc(sizeof(bool));
	*ret = FALSE;

	if (w != NULL){
		IACTV **i = NULL;	
		for(byte n = 0, num = 0; n < (GLOBALV_MAPW * GLOBALV_MAPH); ++n){
			if (*(w->allchambers + n) != NULL){
				switch(n){
					case 0: num = 2; break;
					default: num = 0;
				};

				i = malloc(sizeof(IACTV *) * num);
				for(byte z = num; z > 0; *(i + z - 1) = iinit(), --z);

				if (i != NULL){
					switch(n){
						case 0:
							(*(w->allchambers + 0))->iactv_setup(*(w->allchambers + 0), num, 
								(*(i + 0))->iload(*(i + 0), "./iactv/d00"), (*(i + 1))->iload(*(i + 1), "./iactv/d01")); 
							break;
					};
				};
				free(i);
				i = NULL;
			};
		};
		*ret = TRUE;
	};
	return ret;
};

GAME *ginit(){
	GAME *g = malloc(sizeof(GAME));
	if (g != NULL){
		g->gsetup = &game_setup;
		g->ginterface = &game_interface;
		g->grefresh = &game_refreshProgress;

		#ifdef DEBUG
			printf("D: will start game main structure...\n");
		#endif

		if ((*g).gsetup(g))
			return g;
		free(g);
	};
	err_exit;
};

WORLD *winit(){
	#ifdef DEBUG
		printf("D: now will create WORLD structure...\n");
	#endif
	WORLD *w = malloc(sizeof(WORLD));
	if (NULL != w){
		w->allchambers = malloc(sizeof(CHAMBER *) * (GLOBALV_MAPW * GLOBALV_MAPH));
		if (NULL != w->allchambers){
			for (byte i = (GLOBALV_MAPW * GLOBALV_MAPH); i > 0; *(w->allchambers + i - 1) = NULL, --i);
			w->chsetup = &chamber_setup;
			w->isetup = &isetup;
			w->wload = &wload;
			w->wgetlabels = &wgetlabels;
			w->nused = 0;
			return w;
		};
		free(w);
	};
	printf("E: \"%s\" failed.\n", __FUNCTION__);
	return NULL;
};

CHAMBER *chinit(){
	CHAMBER *ch = malloc(sizeof(CHAMBER));
	if (NULL != ch){
		#ifdef DEBUG
			printf("D: CHAMBER [%p] structure successfully created.\n", ch);
		#endif
		ch->adjch_setup = &ch_adjch_setup;
		ch->iactv_setup = &ch_iatcv_setup;
		ch->adjchambers = NULL;
		ch->iactives = NULL;
		ch->string = NULL;
		ch->adjnum = 0;
		ch->actnum = 0;
	} else printf("E: failed to start a CHAMBER structure in \"%s\".\n", __FUNCTION__);

	return ch;
};

IACTV *iinit(){
	IACTV *i = malloc(sizeof(IACTV));
	if (i != NULL){
		#ifdef DEBUG
			printf("D: INTERACTIVE [%p] structure successfully created.\n", i);
		#endif
		i->iload = &iload;
		i->script = NULL;
		i->label = NULL;
		i->actions = NULL;
		i->progress = 0;
		i->actnum = 0;
		i->scpnum = 0;
	} else printf("E: failed to start a new INTERACTIVE structure in \"%s\".\n", __FUNCTION__);
	return i;
};

bool idestroy(IACTV **i){
	#ifdef DEBUG
		if((*i)->label != NULL)
			printf("D: INTERACTIVE \"%s\" structure destruction process started...\n", (*i)->label);
		else
			printf("D: INTERACTIVE [%p] structure destruction process started...\n", *i);
	#endif
	if (i != NULL && *i != NULL){
		if ((*i)->label != NULL)
			free((*i)->label);
		if ((*i)->actions != NULL){
			while(0 < (*i)->actnum--)
				if (*((*i)->actions + (*i)->actnum) != NULL)
					free(*((*i)->actions + (*i)->actnum));
			free((*i)->actions);
		};
		if ((*i)->script != NULL){
			while(0 < (*i)->scpnum--)
				if (*((*i)->script + (*i)->scpnum) != NULL)
					free(*((*i)->script + (*i)->scpnum));
			free((*i)->script);
		};
		free(*i);
		(*i) = NULL;
		#ifdef DEBUG
			printf("D: destruction process completed.\n");
		#endif
		return TRUE;
	};
	err_exit;
};

bool chdestroy(CHAMBER **ch){
	#ifdef DEBUG
		printf("D: started CHAMBER [%p] structure destruction process...\n", *ch);
	#endif
	if (ch != NULL && *ch != NULL){
		if ((*ch)->adjchambers != NULL){
			CHAMBER *caux;
			PATH *aux;
			for (byte j = (*ch)->adjnum; j > 0; --j){
				aux = *((*ch)->adjchambers + j - 1);
				if (aux != NULL){
					caux = (aux->b == (*ch)) ? aux->a : aux->b;
					for(byte i = caux->adjnum; i > 0; --i){
						if (*(caux->adjchambers + i - 1) != aux)
							continue;
						*(caux->adjchambers + i - 1) = NULL;
					};
					#ifdef DEBUG
						printf("D: (PATH [%p] will be destroyed)...\n", aux);
					#endif
					free(aux);
				};
			};
			free((*ch)->adjchambers);
		};
		if ((*ch)->iactives != NULL){
			for (byte j = (*ch)->actnum; j > 0; --j)
				if (NULL != *((*ch)->iactives + j - 1))
					idestroy(((*ch)->iactives + j - 1));
			free((*ch)->iactives);
		};
		if ((*ch)->string != NULL)
			free((*ch)->string);
		free(*ch);
		(*ch) = NULL;
		#ifdef DEBUG
			printf("D: success.\n");
		#endif
		return TRUE;
	};
	err_exit;
};

bool wdestroy(WORLD **w){
	#ifdef DEBUG
		printf("D: started WORLD structure destruction process...\n");
	#endif
	if (w != NULL && *w != NULL){
		if((*w)->allchambers != NULL){
			for(byte i = (GLOBALV_MAPW * GLOBALV_MAPH); i > 0; --i)
				if (NULL != *((*w)->allchambers + i - 1))
					chdestroy(((*w)->allchambers + i - 1));
			free((*w)->allchambers);
		};
		free(*w);
		(*w) = NULL;
		#ifdef DEBUG
			printf("D: WORLD structure destruction process completed successfully.\n");
		#endif
		return TRUE;
	};
	err_exit;
};

bool gdestroy(GAME **g){
	#ifdef DEBUG
		printf("D: started GAME structure destruction process...\n");
	#endif
	if (g != NULL && *g != NULL){
		if (NULL != (*g)->world)
			wdestroy(&(*g)->world);
		if (NULL != (*g)->player)
			pdestroy(&(*g)->player);
		if (NULL != (*g)->command)
			cdestroy(&(*g)->command);
		free(*g);
		(*g) = NULL;
		#ifdef DEBUG
			printf("D: GAME structure destruction process completed successfully.\n");
		#endif
		return TRUE;
	};
	err_exit;
};