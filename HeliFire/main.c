/**************************************************/
/*** HELIFIRE *************************************/
/**************************************************/
#include <coleco.h>
#include <getput1.h>

#define chrtab  0x1800 /* écran en linéaire */
#define chrgen  0x0000 /* table des caractères */
#define coltab  0x2000 /* couleur des caractères */
#define sprtab  0x3800 /* sprite_pattern_table */
#define sprgen  0x1b00 /* sprite_attribute_table */
#define buffer  0x1c00 /* buffer screen 2 */

#define S_TRUE 1
#define S_FALSE 0

#define INACTIF 204
#define INVISIBLE 203

#define HAUT		0
#define HAUT_DROITE 1
#define DROITE		2
#define BAS_DROITE	3
#define BAS			4
#define BAS_GAUCHE  5
#define GAUCHE		6
#define	HAUT_GAUCHE	7


#define EN_HELICO1 1	// helicoptère déplacement horizontal vitesse 1 gauche
#define EN_HELICO2 2	// helicoptère déplacement horizontal vitesse 2 gauche
#define EN_HELICO3 3	// helicoptère déplacement horizontal vitesse 3 gauche
#define EN_HELICROSS 4	// hélicoptère avec mouvement croisé
#define EN_HELISQUARE 5	// Hélicoptère avec mouvement carré
#define EN_HELICO1_RIGHT 6	// helicoptère déplacement horizontal vitesse 1 droite
#define EN_HELICO2_RIGHT 7	// helicoptère déplacement horizontal vitesse 2 droite
#define EN_HELICO3_RIGHT 8	// helicoptère déplacement horizontal vitesse 3 droite
#define EN_SUBENNEMY	50 // Sous marin ennemy
#define EN_BOAT			51 // Bateau ennemy
#define EN_SHOOTVERT 101	// Tir ennemie de base vertical
#define EN_SHOOTHORI 102	// Tir ennemie de base horizontal
#define EN_MINE 103			// "Mine" ennemie de base vertical
#define EN_MINESHOOT 104	// "Mine" lanceur de missile horizontal
#define EN_TORPILLE 105		// Torpille horizontale rapide
#define EN_MISSMINE	106		// Missile des mines
#define EN_TORPILLE2 107		// Torpille horizontale rapide
#define	EN_PLOUFSHOOT	150		// Plouf du shoot
#define	EN_PLOUFMINE	151		// Plouf de la mine
#define	EN_EXPLOSION	152		// Explosion Helico
#define	EN_SUBEXPLOSION	153		// Explosion Sous-Marin


#define MAXENNEMYSPRITE 25 	/* Nombre d'ennemies et de tir ennemis en sprite maximum*/

#define W_SPLASH0	0
#define W_SPLASH1	1
#define	W_INIT		2
#define W_MENU		3
#define W_INGAME	4
#define W_GAMEOVER	5
#define W_LOOSELIFE	6

extern const byte allWater;
extern const byte NAMERLE[];
extern const byte PATTERNRLE[];
extern const byte COLORRLE[];
extern const byte SPATTERNRLE[];

extern const byte SPLASH_NAMERLE[];
extern const byte SPLASH_PATTERNRLE[];
extern const byte SPLASH_COLORRLE[];

extern const byte TITLE_PATTERNRLE[];
extern const byte TITLE_COLORRLE[];

extern const sound_t snd_table[] ;
extern const void music[];

const byte helifire_title[] = {
 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
 92, 93, 96, 99,102,105,108,111,114,117,120,114,123,126,129,132, 92,
 92, 94, 97,100,103,106,109,112,115,118,121,115,124,127,130,133, 92,
 92, 95, 98,101,104,107,110,113,116,119,122,116,125,128,131,134, 92,
 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92
};

// Variables pour gestion du flickering
char ticks;
byte currentFlicker;
sprite_t bsprites[32];
byte odd;
byte scrollVague_index;
byte refresh_score;
byte refresh_info;
byte nbPlayers;
byte workflow;
byte doScroll;

byte player_loose; /* Quel est le joueur qui à perdu une vie ? */


unsigned bigTimer;



byte timer8;
byte timer24;



byte subAttack;

//byte myDelay;

byte computeNewWave;

byte level;  // Level courant : 0 à 3
byte loop;	// Loop du jeu (loop = 1, on à passé une fois déja les 4 niveaux)
byte nbHelicoMaxLevel;
byte nbHelicoGenerated;
byte nbHelicoCurrent;

byte ligneHelico[14]; /* Pas plus d'un hélico par ligne virtuelle */

byte p_source;
byte nb_source;

typedef struct
{
	byte sprno;	
	unsigned score;	
	byte nblives;
} s_player;

// 2 Joueurs
s_player player[2];

typedef struct
{
	//byte actif;
	byte sprno;
} s_shoot;

// 1 Shoot par joueur
s_shoot shoot[2];

typedef struct
{
	byte type; // Type d'ennemie (INACTIF possible)
	byte sprno;
	byte timer;			// Timer qui va servir à pas mal de chose ...
						// Temps d'affichage des animations, ou génération des animations
	byte speed;			// Speed = 1 : Petit coup de speed en plus pour les hélicos
	byte direction;		// Variable direction pour certaines IA
	byte duree;			// Variable duréee pour certaines IA
} s_ennemySprite;

s_ennemySprite ennemySprite[MAXENNEMYSPRITE];

typedef struct
{
	char x;
	char y;
	byte p;
} s_star;

s_star stars[7];

void generateRandomStars()
{
	byte i;
	byte px;
	
	px = 0;
	
	for (i=0;i<7;i++)
	{
		stars[i].x = px;
		px+=4;
		stars[i].y = rnd_byte(2,8);
		stars[i].p = 176;
	}
}

void initLevel(byte l)
{
	byte i;
	
	nbHelicoCurrent = 0;
	nbHelicoGenerated = 0;
	computeNewWave=S_FALSE;
	level = l;
	
	// 10 Hélicos apparaissent pour les level 0 à 2 quelquesoit le loop
	//if ( (l==0) || (l==1) || (l==2) ) nbHelicoMaxLevel = 10;
	// 4 Hélicos apparaissent pour le level 4 quelquesoit le loop
	//if (l==3) || (l==nbHelicoMaxLevel = 4;
	
	if ((l&1)==0)
		nbHelicoMaxLevel = 5;
	else
		nbHelicoMaxLevel = 10;
	
	// Mise à vide des lignes d'apparition des hélicos. 1 seul hélico par ligne
	for (i=0;i<12;i++) ligneHelico[i] = 0;
	refresh_info=1;
}

void showAndMoveStars()
{
	byte i;
	s_star *this_star;
	byte *star_x;
	byte *star_y;
	byte *star_p;
	
	for (i=0;i<7;i++)
	{
		this_star = &stars[i];
		star_x = &this_star->x;
		star_y = &this_star->y;
		star_p = &this_star->p;
		
		(*star_p) ++;		

		if ((*star_p)==184)
		{
			put_char((*star_x),stars[i].y,(*star_p));
			if (((*star_x)+1)<32) put_char((*star_x)+1,(*star_y),176);
			(*star_p) = 176;
			(*star_x) ++;
			if ((*star_x)>31) (*star_x) = 0;
		}
		else
		{
			put_char((*star_x),(*star_y),(*star_p));
		}
	}
}

void activateSprite(byte s,byte x,byte y,byte p,byte c)
{
	sprites[s].x = x;
	sprites[s].y = y;
	sprites[s].pattern = p;
	sprites[s].colour = c;
}

byte isCollision(byte x1,byte y1,byte h1,byte l1,byte x2,byte y2,byte h2,byte l2)
{	
	if(x1 > x2+l2) return S_FALSE;
	if(x1+l1 < x2) return S_FALSE;
	if(y1 > y2+h2) return S_FALSE;
	if(y1+h1 < y2) return S_FALSE;
	
	return S_TRUE;
}

char getFreeSprite()
{
	char i;
	
	if (odd==0)
	{
		odd = 1;
		for (i=0;i<32;i++)
		{
			if (sprites[i].y==INACTIF) 
			{
				return i;
			}
		}
	}
	else
	{
		odd = 0;
		for (i=31;i>0;i--)
		{
			if (sprites[i].y==INACTIF) 
			{
				return i;
			}
		}
	}
	
	// Si il n'y à plus de sprite de libre, on va prendre un sprite moins prioritaire !
	for (i=0;i<MAXENNEMYSPRITE;i++)
	{
		if ( (ennemySprite[i].type==EN_SHOOTVERT) || ( ennemySprite[i].type==EN_PLOUFSHOOT) || (ennemySprite[i].type==EN_PLOUFMINE) || (ennemySprite[i].type==EN_EXPLOSION ) )
		{
			ennemySprite[i].type = INACTIF;
			sprites[ennemySprite[i].sprno].y = INACTIF;
			return ennemySprite[i].sprno; // Retourne le n° du sprite récupéré
		}
	}
	
	return -1;
}

unsigned char returnIfObjectExist(unsigned char typeObj)
{
	unsigned char i;
	unsigned char r;
	
	r=0;
	
	for (i=0;i<MAXENNEMYSPRITE;i++) 
		if (ennemySprite[i].type == typeObj) r++;
		
	return r;
}

char getFreeEnnemy()
{
	char i;
	
	for (i=0;i<MAXENNEMYSPRITE;i++) 
		if (ennemySprite[i].type == INACTIF) return i;
		
	// Si il n'y à plus de sprite de libre, on va prendre un sprite moins prioritaire !
	for (i=0;i<MAXENNEMYSPRITE;i++)
	{
		if ( (ennemySprite[i].type==EN_SHOOTVERT) ||( ennemySprite[i].type==EN_PLOUFSHOOT) || (ennemySprite[i].type==EN_PLOUFMINE) || (ennemySprite[i].type==EN_EXPLOSION ) )
		{
			ennemySprite[i].type = INACTIF;
			sprites[ennemySprite[i].sprno].y = INACTIF;
			return i; // Retourne le n° de l'ennemie récupéré
		}
	}
	
	return -1;
}

// t = type de l'ennemi
// p = pattern de base
// c = couleur de base
void createEnnemyForWave(byte t,byte p,byte c)
{
	char fs,fe;
	byte l;
	
	fe = getFreeEnnemy();
	fs = getFreeSprite();
	if ( (fs!=-1) && (fe!=-1) )
	{
		ennemySprite[fe].type = t;
		ennemySprite[fe].sprno = fs;
		ennemySprite[fe].speed = rnd_byte(0,1);
		ennemySprite[fe].timer = rnd_byte(8,64);
		ennemySprite[fe].direction = BAS_GAUCHE;
		ennemySprite[fe].duree = (240-16);
		
		
		// Helico avec simple déplacement Horizontal
		if ((t!=EN_HELICROSS) && (t!=EN_HELISQUARE))
		{
			l = rnd_byte(0,11);
			while (ligneHelico[l]==1) l = rnd_byte(0,13);
					
			ligneHelico[l] = 1;
			activateSprite(fs,255,(l<<2)+20,p,c);
		}
		else
		{
			activateSprite(fs,255-16,24,p,c);
		}
		
		nbHelicoGenerated++;	
		nbHelicoCurrent++;
	}				
}

void generateEnnemySprite(byte x,byte y,byte t,byte p,byte c)
{
	char e,s;
	
	e = getFreeEnnemy();
	s = getFreeSprite();
	if ( (e!=-1) && (s!=-1) )
	{
		ennemySprite[e].sprno = s;
		ennemySprite[e].type = t;
		activateSprite(s,x,y,p,c);
		
		if ( (t==EN_SHOOTVERT) || (t==EN_MINE) || (t==EN_MINESHOOT) ) {ennemySprite[e].timer = 1;play_sound(6); }
	}
}

// Générateur d'ennemies
void generateEnnemy()
{
	char tmp,r,choose_player;
	tmp=0;
	/*****************************************************************************/
	/* VAGUE DU NIVEAU 0 *********************************************************/
	/*****************************************************************************/
	// Si on est au niveau 0
	if ( (level==0) || (level==1) )
	{
		// , et qu'on à pas encore générer le nombre maximum d'hélico
		if ( (nbHelicoMaxLevel!=nbHelicoGenerated) && ((bigTimer&63)==0 ) )
		{
				computeNewWave = S_FALSE;
				createEnnemyForWave(EN_HELICO1,4,7);				
		}
	}
	else
	/*****************************************************************************/
	/* VAGUE DU NIVEAU 1 *********************************************************/
	/*****************************************************************************/	
	if ( (level==2) || (level==3) )
	{
		if ( (nbHelicoMaxLevel!=nbHelicoGenerated) && ((bigTimer&15)==0 ) )
		{
				computeNewWave = S_FALSE;
				createEnnemyForWave(EN_HELICO2,4,3);				
		}
	}
	else
	/*****************************************************************************/
	/* VAGUE DU NIVEAU 2 *********************************************************/
	/*****************************************************************************/	
	if ( (level==4) || (level==5) )
	{
		if ( (nbHelicoMaxLevel!=nbHelicoGenerated) && ((bigTimer&15)==0 ) )
		{
				computeNewWave = S_FALSE;
				createEnnemyForWave(EN_HELICO3,4,14);				
		}
	}
	else
	/*****************************************************************************/
	/* VAGUE DU NIVEAU 3 *********************************************************/
	/*****************************************************************************/	
	if ( (level==6) || (level==7) )
	{
		if ( (nbHelicoMaxLevel!=nbHelicoGenerated) && ((bigTimer&63)==0 ) )
		{
				computeNewWave = S_FALSE;
				createEnnemyForWave(EN_HELISQUARE,4,7);				
		}
	}
	else
	/*****************************************************************************/
	/* VAGUE DU NIVEAU 4 *********************************************************/
	/*****************************************************************************/	
	if ( (level==8) || (level==9) )
	{
		if ( (nbHelicoMaxLevel!=nbHelicoGenerated) && ((bigTimer&63)==0 ) )
		{
				computeNewWave = S_FALSE;
				createEnnemyForWave(EN_HELICROSS,4,7);
		}
	}
	else
	/*****************************************************************************/
	/* VAGUE DU NIVEAU 5 *********************************************************/
	/*****************************************************************************/	
	if ( (level==10) || (level==11) )
	{
		if ( (nbHelicoMaxLevel!=nbHelicoGenerated) && ((bigTimer&63)==0 ) )
		{
				r=rnd_byte(0,100);
				computeNewWave = S_FALSE;
				if (r<50)
					createEnnemyForWave(EN_HELICROSS,4,7);
				else
					createEnnemyForWave(EN_HELISQUARE,4,7);
		}
	}

	// Si on à générer le maximum d'hélico possible, si tout est détruit
	// on pourra passer à la vague suivante.
	if (nbHelicoMaxLevel==nbHelicoGenerated) computeNewWave = S_TRUE;
	
	/* Tout les 8 missiles tirés on va générer une torpille ! */
	
	if (nbPlayers==1)
	{
		if ((subAttack&15)==0)
		{		
				if (sprites[player[0].sprno].x<120)
					generateEnnemySprite(0,sprites[player[0].sprno].y+10,EN_TORPILLE2,72,15);
				else
					generateEnnemySprite(255,sprites[player[0].sprno].y+10,EN_TORPILLE,28,15);
					
				tmp=1;
				
				play_sound(5);
		}
		
				
	}
	else
	if (nbPlayers==2)
	{
		if (rnd_byte(0,100)<50) choose_player=0; else choose_player=1;
		
		if ( (choose_player==0) && (player[choose_player].nblives==0)) choose_player=1;
		if ( (choose_player==1) && (player[choose_player].nblives==0)) choose_player=0;
		
		
		if ((subAttack&7)==0)
		{		
			if (rnd_byte(0,100)<50)
				generateEnnemySprite(0,sprites[player[choose_player].sprno].y+10,EN_TORPILLE2,72,15);
			else
				generateEnnemySprite(255,sprites[player[choose_player].sprno].y+10,EN_TORPILLE,28,15);
			
			tmp=1;
			
			play_sound(5);
		}	
	}
	
	if ((subAttack&31)==0)
	{	
		generateEnnemySprite(0,80+16+8,EN_SUBENNEMY,56,10);
		generateEnnemySprite(255,80+8,EN_BOAT,48,2);
		
		//tmp=1;
	}
	
	if (tmp==1) subAttack++;
}

// Version non optimisée.
void moveEnnemySprite()
{
	byte i,ran;
	char e,s;
	unsigned char tmp;
	
	s_ennemySprite *this_ennemySprite;
	byte *ennemySprite_type;
	byte *ennemySprite_sprno;
	byte *ennemySprite_timer;
	byte *ennemySprite_speed;
	byte *ennemySprite_duree;
	byte *ennemySprite_direction;
	
	sprite_t *this_ennemySprite_sprite;
	byte *ennemySprite_sprite_x;
	byte *ennemySprite_sprite_y;
	byte *ennemySprite_sprite_p;
	
	for (i=0;i<MAXENNEMYSPRITE;i++)
	{
		this_ennemySprite = &ennemySprite[i];
		ennemySprite_type = &this_ennemySprite->type;
		ennemySprite_sprno = &this_ennemySprite->sprno;
		ennemySprite_timer = &this_ennemySprite->timer;
		ennemySprite_speed = &this_ennemySprite->speed;
		
		this_ennemySprite_sprite = &sprites[(*ennemySprite_sprno)];
		ennemySprite_sprite_x = &this_ennemySprite_sprite->x;
		ennemySprite_sprite_y = &this_ennemySprite_sprite->y;
		ennemySprite_sprite_p = &this_ennemySprite_sprite->pattern;
		
		/***********************************************/
		/* IA HELICO 1 *********************************/
		/***********************************************/
		if ((*ennemySprite_type) == EN_HELICO1)
		{
			// MOUVEMENT
			if ((bigTimer&3)==0)
			{
				(*ennemySprite_sprite_x)--;
				if (*ennemySprite_timer>0) (*ennemySprite_timer)--; else (*ennemySprite_timer)=0;
				if ( ((*ennemySprite_speed)==1) && (timer8==0) ) (*ennemySprite_sprite_x)--;
				if ((*ennemySprite_sprite_x)<5) 
				{
					(*ennemySprite_sprite_x) = 255;
				}							
			}
			
			// ANIMATION
			if (timer8>4) (*ennemySprite_sprite_p) = 8; else (*ennemySprite_sprite_p) = 4;			
			
			
			// TIR
			if ((*ennemySprite_timer)==0)
			{
				ran = rnd(0,100);
				if (ran<80) generateEnnemySprite((*ennemySprite_sprite_x),(*ennemySprite_sprite_y)+10,EN_SHOOTVERT,20,15);	
				else
				{
					tmp = returnIfObjectExist(EN_MINESHOOT);
					if ((tmp==0) && ((*ennemySprite_sprite_x)>240))
						// Si on est en bonne position et qu'il n'y à pas de MINESHOOT, on en crée une
						generateEnnemySprite((*ennemySprite_sprite_x),(*ennemySprite_sprite_y)+10,EN_MINESHOOT,36,2);
						else
						{
							tmp = returnIfObjectExist(EN_MINE);
							if (tmp<3)
							{
								generateEnnemySprite((*ennemySprite_sprite_x),(*ennemySprite_sprite_y)+10,EN_MINE,36,15);
							}
						}
					
				}
						
				(*ennemySprite_timer) = 200-(loop<<4)-(loop<<2); // Un tir tout les n pixels selon difficultée
			}
	
		}
		else
		/***********************************************/
		/* IA HELICO 2 *********************************/
		/***********************************************/
		if ((*ennemySprite_type) == EN_HELICO2)
		{
			// MOUVEMENT
			if ((bigTimer&1)==0)
			{
				(*ennemySprite_sprite_x)--;
				if (*ennemySprite_timer>0) (*ennemySprite_timer)--; else (*ennemySprite_timer)=0;
				if ( ((*ennemySprite_speed)==1) && (timer8==0) ) (*ennemySprite_sprite_x)--;
				if ((*ennemySprite_sprite_x)<5) 
				{
					(*ennemySprite_sprite_x) = 255;
				}							
			}
			
			// ANIMATION
			if (timer8>4) (*ennemySprite_sprite_p) = 8; else (*ennemySprite_sprite_p) = 4;	

			// TIR
			// TIR
			if ((*ennemySprite_timer)==0)
			{
				ran = rnd(0,100);
				if (ran<80) generateEnnemySprite((*ennemySprite_sprite_x),(*ennemySprite_sprite_y)+10,EN_SHOOTVERT,20,15);	
				else
				{
					tmp = returnIfObjectExist(EN_MINESHOOT);
					if ((tmp==0) && ((*ennemySprite_sprite_x)>224))
						// Si on est en bonne position et qu'il n'y à pas de MINESHOOT, on en crée une
						generateEnnemySprite((*ennemySprite_sprite_x),(*ennemySprite_sprite_y)+10,EN_MINESHOOT,36,2);
						else
						{
							tmp = returnIfObjectExist(EN_MINE);
							if (tmp<3)
							{
								generateEnnemySprite((*ennemySprite_sprite_x),(*ennemySprite_sprite_y)+10,EN_MINE,36,15);
							}
						}
					
				}
						
				(*ennemySprite_timer) = 240-(loop<<4)-(loop<<2); // Un tir tout les n pixels selon difficultée
			}
		}
		else
		/***********************************************/
		/* IA HELICO 3 *********************************/
		/***********************************************/
		if ((*ennemySprite_type) == EN_HELICO3)
		{
			// MOUVEMENT
			if ((bigTimer&1)==0)
			{
				(*ennemySprite_sprite_x)--;
				if (*ennemySprite_timer>0) (*ennemySprite_timer)--; else (*ennemySprite_timer)=0;
				if ( ((*ennemySprite_speed)==1) && (timer8==0) ) (*ennemySprite_sprite_x)-=2;
				if ((*ennemySprite_sprite_x)<5) 
				{
					(*ennemySprite_sprite_x) = 255;
				}							
			}
			
			// ANIMATION
			if (timer8>4) (*ennemySprite_sprite_p) = 8; else (*ennemySprite_sprite_p) = 4;	

			// TIR
			if ((*ennemySprite_timer)==0)
			{
				ran = rnd(0,100);
				if (ran<80) generateEnnemySprite((*ennemySprite_sprite_x),(*ennemySprite_sprite_y)+10,EN_SHOOTVERT,20,15);	
				else
				{
					tmp = returnIfObjectExist(EN_MINESHOOT);
					if ((tmp==0) && ((*ennemySprite_sprite_x)>208))
						// Si on est en bonne position et qu'il n'y à pas de MINESHOOT, on en crée une
						generateEnnemySprite((*ennemySprite_sprite_x),(*ennemySprite_sprite_y)+10,EN_MINESHOOT,36,2);
						else
						{
							tmp = returnIfObjectExist(EN_MINE);
							if (tmp<3)
							{
								generateEnnemySprite((*ennemySprite_sprite_x),(*ennemySprite_sprite_y)+10,EN_MINE,36,15);
							}
						}
					
				}
						
				(*ennemySprite_timer) = 240-(loop<<4)-(loop<<2); // Un tir tout les n pixels selon difficultée
			}
		}
		else
		/***********************************************/
		/* IA HELISQUARE   *****************************/
		/***********************************************/
		if ((*ennemySprite_type) == EN_HELISQUARE)
		{
			ennemySprite_duree = &this_ennemySprite->duree;
			ennemySprite_direction = &this_ennemySprite->direction;

			// MOUVEMENT
			if ((bigTimer&1)==0)
			{
				if (*ennemySprite_timer>0) (*ennemySprite_timer)--; else (*ennemySprite_timer)=0;
			}
			
			/* Déplacement vers la gauche */
			if ((*ennemySprite_sprite_y)==24)  {(*ennemySprite_sprite_x)--;(*ennemySprite_direction)=GAUCHE;}
			/* Déplacement vers la droite */
			if ((*ennemySprite_sprite_y)==80)  {(*ennemySprite_sprite_x)++;(*ennemySprite_direction)=DROITE;}			
			
			/* Déplacement vers le bas */
			if (((*ennemySprite_sprite_x)==8) && ((*ennemySprite_sprite_y)!=80))   (*ennemySprite_sprite_y)++;
			/* Déplacement vers le haut */
			if (((*ennemySprite_sprite_x)==240) && ((*ennemySprite_sprite_y)!=24) )   (*ennemySprite_sprite_y)--;
			
			// ANIMATION
			if ((*ennemySprite_direction)==GAUCHE)
			{
				if (timer8>4) (*ennemySprite_sprite_p) = 8; else (*ennemySprite_sprite_p) = 4;	
			}
			else if ((*ennemySprite_direction)==DROITE)
			{
				if (timer8>4) (*ennemySprite_sprite_p) = 12; else (*ennemySprite_sprite_p) = 16;	
			}
			
			// TIR
			if ((*ennemySprite_timer)==0)
			{
				ran = rnd(0,100);
				if (ran<80) generateEnnemySprite((*ennemySprite_sprite_x),(*ennemySprite_sprite_y)+10,EN_SHOOTVERT,20,15);	
				else
				{
					tmp = returnIfObjectExist(EN_MINESHOOT);
					if ((tmp==0) && ((*ennemySprite_sprite_x)>208))
						// Si on est en bonne position et qu'il n'y à pas de MINESHOOT, on en crée une
						generateEnnemySprite((*ennemySprite_sprite_x),(*ennemySprite_sprite_y)+10,EN_MINESHOOT,36,2);
						else
						{
							tmp = returnIfObjectExist(EN_MINE);
							if (tmp<3)
							{
								generateEnnemySprite((*ennemySprite_sprite_x),(*ennemySprite_sprite_y)+10,EN_MINE,36,15);
							}
						}
					
				}
						
				(*ennemySprite_timer) = 240-(loop<<4)-(loop<<2); // Un tir tout les n pixels selon difficultée
			}
		}		
		else
		/***********************************************/
		/* IA HELISCROSS   *****************************/
		/***********************************************/
		if ((*ennemySprite_type) == EN_HELICROSS)
		{
			ennemySprite_duree = &this_ennemySprite->duree;
			ennemySprite_direction = &this_ennemySprite->direction;
			
			// MOUVEMENT
			if ((bigTimer&1)==0)
			{
				if (*ennemySprite_timer>0) (*ennemySprite_timer)--; else (*ennemySprite_timer)=0;
			}
			
			// On doit faire un mouvement
			if (*ennemySprite_duree>0)
			{
				if (*ennemySprite_direction==BAS_GAUCHE) {(*ennemySprite_sprite_x)--;if ((bigTimer&3)==0) (*ennemySprite_sprite_y)++;}
				else
				if (*ennemySprite_direction==HAUT) {(*ennemySprite_sprite_y)--;}
				else
				if (*ennemySprite_direction==BAS_DROITE) {(*ennemySprite_sprite_x)++;if ((bigTimer&3)==0) (*ennemySprite_sprite_y)++;}
				
				(*ennemySprite_duree)--;
			}
			else
			{
				if (*ennemySprite_direction==BAS_GAUCHE) {(*ennemySprite_direction)=HAUT;(*ennemySprite_duree)=(*ennemySprite_sprite_y)-16;}
				else
				if (*ennemySprite_direction==BAS_DROITE) {(*ennemySprite_direction)=HAUT;(*ennemySprite_duree)=(*ennemySprite_sprite_y)-16;}
				else
				if (*ennemySprite_direction==HAUT)
				{
					if ((*ennemySprite_sprite_x)<100)
					{
						(*ennemySprite_direction)=BAS_DROITE;(*ennemySprite_duree)=(240-16);
					}
					else
					{
						(*ennemySprite_direction)=BAS_GAUCHE;(*ennemySprite_duree)=(240-16);
					}
				}				
			}
			
			// ANIMATION
			if ((*ennemySprite_direction)==BAS_GAUCHE)
			{
				if (timer8>4) (*ennemySprite_sprite_p) = 8; else (*ennemySprite_sprite_p) = 4;	
			}
			else if ((*ennemySprite_direction)==BAS_DROITE)
			{
				if (timer8>4) (*ennemySprite_sprite_p) = 12; else (*ennemySprite_sprite_p) = 16;	
			}
			
			// TIR
			if ((*ennemySprite_timer)==0)
			{
				ran = rnd(0,100);
				if (ran<80) generateEnnemySprite((*ennemySprite_sprite_x),(*ennemySprite_sprite_y)+10,EN_SHOOTVERT,20,15);	
				else
				{
					tmp = returnIfObjectExist(EN_MINESHOOT);
					if ((tmp==0) && ((*ennemySprite_sprite_x)>208))
						// Si on est en bonne position et qu'il n'y à pas de MINESHOOT, on en crée une
						generateEnnemySprite((*ennemySprite_sprite_x),(*ennemySprite_sprite_y)+10,EN_MINESHOOT,36,2);
						else
						{
							tmp = returnIfObjectExist(EN_MINE);
							if (tmp<3)
							{
								generateEnnemySprite((*ennemySprite_sprite_x),(*ennemySprite_sprite_y)+10,EN_MINE,36,15);
							}
						}
					
				}
						
				(*ennemySprite_timer) = 240-(loop<<4)-(loop<<2); // Un tir tout les n pixels selon difficultée
			}
		}				
		else
		/***********************************************/
		/* IA SHOOT ENNEMY VERTICAL ********************/
		/***********************************************/		
		if ((*ennemySprite_type) == EN_SHOOTVERT)
		{
			if ((bigTimer&1)==0)
			{
				(*ennemySprite_sprite_y)+=2;
				
				/*******************/
				/* GENERE LE PLOUF */
				/*******************/
				if (((*ennemySprite_sprite_y)>90)  && ((*ennemySprite_timer)==1) )
				{
					e = getFreeEnnemy();
					s = getFreeSprite();
					if ( (e!=-1) && (s!=-1) )
					{
						ennemySprite[e].sprno = s;
						ennemySprite[e].type = EN_PLOUFSHOOT;
						ennemySprite[e].timer = 30;
						(*ennemySprite_timer) = 0;
						activateSprite(s,(*ennemySprite_sprite_x),(*ennemySprite_sprite_y),52,15);
					}
				}
				
				/********************/
				/* HORS DE L'ECRAN **/
				/********************/
				if ((*ennemySprite_sprite_y)>168) 
				{
					(*ennemySprite_sprite_y) = INACTIF;
					(*ennemySprite_type) = INACTIF;
				}							
			}						
		}
		else
		/***********************************************/
		/* IA EXPLOSION + PLOUFSHOOT + PLOUFMINE *******/
		/***********************************************/		
		if ( ((*ennemySprite_type) > (EN_PLOUFSHOOT-1)) &&  ((*ennemySprite_type) < (EN_EXPLOSION+1)) )
		{
			(*ennemySprite_timer)--;
			if ((*ennemySprite_timer)==0)
			{
				(*ennemySprite_type) = INACTIF;
				(*ennemySprite_sprite_y) = INACTIF;
				subAttack++;
			}
		}
		else
		/*********************************************************/
		/* IA SHOOT HORIZONTAL + TORPILLE  ***********************/
		/*********************************************************/		
		if (((*ennemySprite_type) == (EN_SHOOTHORI)) || ((*ennemySprite_type) == (EN_TORPILLE)))
		{
			
			if ((*ennemySprite_type) == (EN_TORPILLE)) (*ennemySprite_sprite_x)-=3;
			else (*ennemySprite_sprite_x)--;
			
			if ((*ennemySprite_sprite_x)<4)
			{
				(*ennemySprite_type) = INACTIF;
				(*ennemySprite_sprite_y) = INACTIF;
			}
		}
		else
		/*********************************************************/
		/* TORPILLE 2 ***********************/
		/*********************************************************/		
		if (((*ennemySprite_type) == (EN_TORPILLE2)))
		{
			
			(*ennemySprite_sprite_x)+=3;
			
			
			if ((*ennemySprite_sprite_x)>249)
			{
				(*ennemySprite_type) = INACTIF;
				(*ennemySprite_sprite_y) = INACTIF;
			}
		}
		/*********************************************************/
		/* SUB ***********************/
		/*********************************************************/		
		if (((*ennemySprite_type) == (EN_SUBENNEMY)))
		{
			
			if ((bigTimer&1)==0)(*ennemySprite_sprite_x)+=1;
			
			if ((*ennemySprite_sprite_x)>249)
			{
				(*ennemySprite_type) = INACTIF;
				(*ennemySprite_sprite_y) = INACTIF;
			}
		}				
		else
		/*********************************************************/
		/* BOAT ***********************/
		/*********************************************************/		
		if (((*ennemySprite_type) == (EN_BOAT)))
		{
			
			if ((bigTimer&1)==0)(*ennemySprite_sprite_x)-=1;
			
			
			if ((*ennemySprite_sprite_x)<2)
			{
				(*ennemySprite_type) = INACTIF;
				(*ennemySprite_sprite_y) = INACTIF;
			}
		}				
		else
		/***********************************************/
		/* MISSMINE *************************/
		/***********************************************/
		if ((*ennemySprite_type) == EN_MISSMINE)
		{
			(*ennemySprite_sprite_x)--;
			if ((bigTimer&3)==0) (*ennemySprite_sprite_x)--;
			if ((*ennemySprite_sprite_x)<5) 
			{
				(*ennemySprite_sprite_y) = INACTIF;
				(*ennemySprite_type) = INACTIF;
			}							
		}
		else
		/***********************************************/
		/* IA MINE + MINESHOOT *************************/
		/***********************************************/		
		if ( ((*ennemySprite_type) == EN_MINE) || ((*ennemySprite_type) == EN_MINESHOOT) )
		{
			if (timer8==0)
			{
				(*ennemySprite_sprite_y)++;
				if (((bigTimer&15)==0) && ((*ennemySprite_sprite_y)<90) ) (*ennemySprite_sprite_y)+=2;
				 
				if ((*ennemySprite_sprite_y)>168) 
				{
					(*ennemySprite_sprite_y) = INACTIF;
					(*ennemySprite_type) = INACTIF;
				}	
				
				/*******************/
				/* GENERE LE PLOUF */
				/*******************/
				if (((*ennemySprite_sprite_y)>90)  && ((*ennemySprite_timer)==1) )
				{
					e = getFreeEnnemy();
					s = getFreeSprite();
					if ( (e!=-1) && (s!=-1) )
					{
						ennemySprite[e].sprno = s;
						ennemySprite[e].type = EN_PLOUFMINE;
						ennemySprite[e].timer = 30;
						(*ennemySprite_timer) = 0;
						activateSprite(s,(*ennemySprite_sprite_x),(*ennemySprite_sprite_y),44,15);
					}
				}
				
				/********************/
				/* HORS DE L'ECRAN **/
				/********************/				
			}

			if ( (timer8>4) && ((*ennemySprite_sprite_y)>90) ) (*ennemySprite_sprite_p) = 36; else (*ennemySprite_sprite_p) = 40;
			
			if ((*ennemySprite_type) == EN_MINESHOOT) 
			{	
				if ((bigTimer&7)==0) (*ennemySprite_sprite_y)++;
				if ((timer24==0) && (*ennemySprite_sprite_y>96))
				{
					generateEnnemySprite((*ennemySprite_sprite_x),(*ennemySprite_sprite_y),EN_MISSMINE,32,15);
					play_sound(2);
				}
				// TODO : Faire tirer la mine Horizontalement !
			}
		}		
	}
}

byte checkCollision()
{
	byte i,j;
	char e,s;

	sprite_t *this_player_sprite;
	byte *player_sprite_x;
	byte *player_sprite_y;
	
	sprite_t *this_shoot_sprite;
	byte *shoot_sprite_x;
	byte *shoot_sprite_y;

	s_ennemySprite *this_ennemySprite;
	byte *ennemySprite_type;
	byte *ennemySprite_sprno;
	
	sprite_t *this_ennemySprite_sprite;
	byte *ennemySprite_sprite_x;
	byte *ennemySprite_sprite_y;
		
	//if (vdp_status==159) return S_FALSE;
	
	for (i=0;i<nbPlayers;i++)
	{
		this_shoot_sprite = &sprites[shoot[i].sprno];
		shoot_sprite_x = &this_shoot_sprite->x;
		shoot_sprite_y = &this_shoot_sprite->y;
		
		this_player_sprite = &sprites[player[i].sprno];
		player_sprite_x = &this_player_sprite->x;
		player_sprite_y = &this_player_sprite->y;
		
		//if (i==0) {center_string(0,str((*player_sprite_x)));center_string(1,str((*player_sprite_y)));}
		
		
		//if (((*shoot_sprite_y)!=INVISIBLE) && ((*shoot_sprite_y)<88))
		//{
			//myDelay = 0;
			for (j=0;j<MAXENNEMYSPRITE;j++)
			{
				this_ennemySprite = &ennemySprite[j];
				ennemySprite_type = &this_ennemySprite->type;
				ennemySprite_sprno = &this_ennemySprite->sprno;
				
				this_ennemySprite_sprite = &sprites[(*ennemySprite_sprno)];
				ennemySprite_sprite_x = &this_ennemySprite_sprite->x;
				ennemySprite_sprite_y = &this_ennemySprite_sprite->y;
				
				if (((*shoot_sprite_y)!=INVISIBLE) && ((*shoot_sprite_y)<88))
				if (((*ennemySprite_type)>(EN_HELICO1-1)) && ((*ennemySprite_type)<(EN_BOAT+1)))
				{
					if (isCollision((*shoot_sprite_x)+7,(*shoot_sprite_y)+11,12,3,(*ennemySprite_sprite_x),(*ennemySprite_sprite_y),8,8) == S_TRUE)
					{					
					
						// On crée une explosion
						e = getFreeEnnemy();
						s = getFreeSprite();
						if ( (e!=-1) && (s!=-1) )
						{
							ennemySprite[e].sprno = s;
							ennemySprite[e].type = EN_EXPLOSION;
							activateSprite(s,(*ennemySprite_sprite_x),(*ennemySprite_sprite_y),68,11);
							ennemySprite[e].timer = 20;
						}

						if ((*ennemySprite_type)==EN_HELICO1) player[i].score+=1;
						if ((*ennemySprite_type)==EN_HELICO2) player[i].score+=2;
						if ((*ennemySprite_type)==EN_HELICO3) player[i].score+=3;
						if ((*ennemySprite_type)==EN_HELISQUARE) player[i].score+=4;
						if ((*ennemySprite_type)==EN_HELICROSS) player[i].score+=5;						
						if ((*ennemySprite_type)==EN_SUBENNEMY) player[i].score+=5;
						if ((*ennemySprite_type)==EN_BOAT) player[i].score+=5;
												
						// On désactive l'ennemie
						if (((*ennemySprite_type)>(EN_HELICO1-1)) && ((*ennemySprite_type)<(EN_HELICO3_RIGHT+1))) nbHelicoCurrent--;
						(*ennemySprite_type)=INACTIF;
						(*ennemySprite_sprite_y)=INACTIF;	
															
						// On rend invisible le shoot sous-marin 
						//sprites[shoot[i].sprno].y = INVISIBLE; 								
						(*shoot_sprite_y) = INVISIBLE;	
						//return S_FALSE;
			
						refresh_score = 1;
						play_sound(4);
					}
				}
								
				if (((*ennemySprite_type)>(EN_SUBENNEMY-1)) && ((*ennemySprite_type)<(EN_TORPILLE2+1)))
				{
					//if (i==0) {center_string(0,str((*player_sprite_x)));center_string(1,str((*player_sprite_y)));}
					if (isCollision((*player_sprite_x),(*player_sprite_y)+10,6,16,(*ennemySprite_sprite_x),(*ennemySprite_sprite_y),4,4) == S_TRUE)
					{
						//pause();
						(*ennemySprite_type)=INACTIF;
						(*ennemySprite_sprite_y)=INACTIF;	
						player_loose = i;
						workflow = W_LOOSELIFE;
						play_sound(4);
						//e = getFreeEnnemy();
						//s = getFreeSprite();						
						
					}
				}
			}
		//}
	}
	
	return S_FALSE;
}

// Teste si on à vider le level
void advanceLevel()
{
	// Si on à générer tout les ennemys possible du level
	if (computeNewWave==S_TRUE)
	{
		// Et si le joueur les a tous détruit
		if (nbHelicoCurrent==0)
		{
			// On passe au level suivant, on incrément d'un loop pour la difficulté
			level++;
			if (level==12) {level=0;if (loop<9) loop++;}
			initLevel(level);
		}
	}
}

void main(void)
{
	byte i,sortie,c;
	byte sx,sy;
	doScroll=0;
	set_snd_table(snd_table);
	stop_music();
	screen_mode_2_bitmap();
	screen_off();
	rle2vram(SPATTERNRLE,sprtab);
	load_patternrle(PATTERNRLE);
	duplicate_pattern();
	rle2vram(COLORRLE,coltab);
	rle2vram(COLORRLE,coltab+2048);
	rle2vram(COLORRLE,coltab+4096);	
	enable_nmi();

	screen_on();

	workflow = W_SPLASH0;
		
	while(1)
	{
		if (workflow==W_SPLASH0)
		{
			doScroll=0;
			disable_nmi();			
			screen_off();
			cls();
			rle2vram(SPLASH_NAMERLE,chrtab);
			load_patternrle(SPLASH_PATTERNRLE);
			duplicate_pattern();
			rle2vram(SPLASH_COLORRLE,coltab);
			rle2vram(SPLASH_COLORRLE,coltab+2048);
			rle2vram(SPLASH_COLORRLE,coltab+4096);							
			screen_on();
			enable_nmi();
			pause();
			workflow=W_SPLASH1;
		}
		else
		if (workflow==W_SPLASH1)
		{
			doScroll=0;
			disable_nmi();			
			screen_off();
			cls();
			load_patternrle(TITLE_PATTERNRLE);
			duplicate_pattern();
			rle2vram(TITLE_COLORRLE,coltab);
			rle2vram(TITLE_COLORRLE,coltab+2048);
			rle2vram(TITLE_COLORRLE,coltab+4096);							
			screen_on();
			enable_nmi();

			
		    play_music(music);
			
			for (i=0;i<12;i++)
			{
				put_frame(helifire_title,7,i,17,5);
				delay(2);
			}			
			pause();		
			for (i=12;i>4;i--)
			{
				put_frame(helifire_title,7,i,17,5);
				delay(2);
			}						
			workflow=W_MENU;			
	
			screen_on();			
		}
		else
		if (workflow==W_MENU)
		{
			while (joypad_1 & FIRE1) {}
			while (joypad_1 & FIRE2) {}
		
			doScroll=0;
			disable_nmi();			
			//screen_off();
			print_at(5,10,"NUMBER OF PLAYERS : 1");
			print_at(5,12,"DIFFICULTY LEVEL : 0");
			print_at(5,14,"      START         ");
			delay(1);
			//screen_on();
			enable_nmi();			
			sortie=0;
			nbPlayers=1;
			loop = 0;
			sx = 3;
			sy = 10;
			while (sortie==0)
			{								
				disable_nmi();
				if (nbPlayers==1) print_at(25,10,"1");
				if (nbPlayers==2) print_at(25,10,"2");
				if (loop==0) print_at(24,12,"0");
				if (loop==1) print_at(24,12,"1");
				if (loop==2) print_at(24,12,"2");
				if (loop==3) print_at(24,12,"3");
				if (loop==4) print_at(24,12,"4");
				if (loop==5) print_at(24,12,"5");
				if (loop==6) print_at(24,12,"6");
				if (loop==7) print_at(24,12,"7");
				if (loop==8) print_at(24,12,"8");
				if (loop==9) print_at(24,12,"9");
				print_at(sx,sy,"*");
				enable_nmi();
				delay(1);				
				
				if (joypad_1 & UP)
				{
					while (joypad_1 & UP);
					play_sound(3);
					disable_nmi();
					print_at(sx,sy," ");
					enable_nmi();
					delay(1);
					if (sy>10) sy-=2;
				}
				if (joypad_1 & DOWN)
				{
					while (joypad_1 & DOWN);
					play_sound(3);
					disable_nmi();
					print_at(sx,sy," ");
					enable_nmi();
					delay(1);
					if (sy<14) sy+=2;				
				}				
				if ( (joypad_1 & FIRE1) || (joypad_1 & FIRE2) )
				{
					while (joypad_1 & FIRE1) {}
					while (joypad_1 & FIRE2) {}
					play_sound(3);
					/* Nombre de joueurs*/
					if (sy==10)
					{
						nbPlayers++;
						if (nbPlayers==3) nbPlayers=1;
					}
					/*Level*/
					if (sy==12)
					{
						loop++;
						if (loop==10) loop=0;
					}
					/*START*/
					if (sy==14) sortie = 1;
				}
			}			
			workflow=W_INIT;
		}
		else
		if (workflow==W_INIT)
		{
			disable_nmi();
			screen_off();
			rle2vram(SPATTERNRLE,sprtab);
			load_patternrle(PATTERNRLE);
			duplicate_pattern();
			rle2vram(COLORRLE,coltab);
			rle2vram(COLORRLE,coltab+2048);
			rle2vram(COLORRLE,coltab+4096);	
			enable_nmi();
			screen_on();
		
			doScroll=0;

			subAttack = 1;
			currentFlicker=0;
			odd = 0;
			bigTimer = 0;
	
			timer8=0;
			timer24=0;
	
	
			level = 0;			
			scrollVague_index=0;	
			refresh_score=1;
			p_source = 0;
			nb_source = 0;
			for (i=0;i<32;i++) sprites[i].y = INACTIF;
			for (i=0;i<MAXENNEMYSPRITE;i++) ennemySprite[i].type = INACTIF;
			generateRandomStars();
			
			// Initialisation joueur
			
			player[0].score = 0;
			player[1].score = 0;
			player[0].nblives = 3;
			if (nbPlayers==2) player[1].nblives = 3; else player[1].nblives=0;
	
			player[0].sprno = getFreeSprite();
			player[1].sprno = getFreeSprite();
			activateSprite(player[0].sprno,100,100,0,7);
			
			if (nbPlayers==2) activateSprite(player[1].sprno,132,100,0,11); else activateSprite(player[1].sprno,100,203,0,11);
			
			// Initialisation du shoot Joueur 1
			shoot[0].sprno = getFreeSprite();
			shoot[1].sprno = getFreeSprite();
			activateSprite(shoot[0].sprno,INVISIBLE,INVISIBLE,24,7);
			activateSprite(shoot[1].sprno,INVISIBLE,INVISIBLE,24,11);
				
			initLevel(level);				
			
			disable_nmi();
			screen_off();
			rle2vram(NAMERLE,chrtab);
			screen_on();
			enable_nmi();

			workflow=W_INGAME;
		}
		else
		if (workflow==W_INGAME)
		{
			doScroll=1;
			
			while(workflow==W_INGAME) 
			{
				ticks=0;
				generateEnnemy();
				if ( (timer8==0) || (timer8==4) )checkCollision();
				
				if (player[0].nblives>0)
				{
					if ((joypad_1 & LEFT) && (sprites[player[0].sprno].x>0) ) sprites[player[0].sprno].x --;
					if ((joypad_1 & RIGHT) && (sprites[player[0].sprno].x<240)) sprites[player[0].sprno].x ++;
					if ((joypad_1 & UP) && (sprites[player[0].sprno].y>82)) sprites[player[0].sprno].y --;
					if ((joypad_1 & DOWN) && (sprites[player[0].sprno].y<150)) sprites[player[0].sprno].y ++;	
					if ( (joypad_1 & FIRE1) || (joypad_1 & FIRE2) )
					{
						if (sprites[shoot[0].sprno].y==INVISIBLE)
						{
							sprites[shoot[0].sprno].x = sprites[player[0].sprno].x;
							sprites[shoot[0].sprno].y = sprites[player[0].sprno].y;
							play_sound(3);
						}
					}
				}

				if ( (nbPlayers==2) && (player[1].nblives>0) )
				{
					if ((joypad_2 & LEFT) && (sprites[player[1].sprno].x>0) ) sprites[player[1].sprno].x --;
					if ((joypad_2 & RIGHT) && (sprites[player[1].sprno].x<240)) sprites[player[1].sprno].x ++;
					if ((joypad_2 & UP) && (sprites[player[1].sprno].y>82)) sprites[player[1].sprno].y --;
					if ((joypad_2 & DOWN) && (sprites[player[1].sprno].y<150)) sprites[player[1].sprno].y ++;	
					if ( (joypad_2 & FIRE1) || (joypad_1 & FIRE2) )
					{
						if (sprites[shoot[1].sprno].y==INVISIBLE)
						{
							sprites[shoot[1].sprno].x = sprites[player[1].sprno].x;
							sprites[shoot[1].sprno].y = sprites[player[1].sprno].y;
							play_sound(3);
						}
					}
				}
				
				for (i=0;i<nbPlayers;i++)
				{
					if (sprites[shoot[i].sprno].y!=INVISIBLE)
					{
						if ((bigTimer&1)==0) sprites[shoot[i].sprno].y -= 3;
						if (sprites[shoot[i].sprno].y<4) 
						{
							sprites[shoot[i].sprno].y = INVISIBLE;
						}
					}
				}
		
				moveEnnemySprite();
					
				bigTimer++;
						
				if (timer8<7) timer8++; else timer8=0;
				if (timer24<24) timer24++; else timer24=0;
				
				advanceLevel(); // Avance d'un level si tout les ennemis sont détruits
				
				if (ticks==0) delay(1);	
			}							
		}
		else
		if (workflow==W_LOOSELIFE)
		{
			doScroll=0;
			/* player_loose, donne le N° du player touché !! */
			c=1;
			for (i=0;i<10;i++)
			{
				sprites[player[player_loose].sprno].colour = c;
				c++;
				if (c==15) c=1;
				delay(2);
			}
			if (player[player_loose].nblives>0) player[player_loose].nblives--;
			
			
			/* REINIT */			
			doScroll=0;
			subAttack = 1;
			currentFlicker=0;
			odd = 0;
			bigTimer = 0;
			timer8=0;
			timer24=0;
			scrollVague_index=0;	
			refresh_score=1;
			p_source = 0;
			nb_source = 0;
			for (i=0;i<32;i++) sprites[i].y = INACTIF;
			for (i=0;i<MAXENNEMYSPRITE;i++) ennemySprite[i].type = INACTIF;
			//generateRandomStars();
			
			// Initialisation joueur
			
			player[0].sprno = getFreeSprite();
			player[1].sprno = getFreeSprite();
			
			if ((player[0].nblives)>0) activateSprite(player[0].sprno,100,100,0,7);
			
			if ((nbPlayers==2) && ((player[1].nblives)>0)) activateSprite(player[1].sprno,132,100,0,11); else activateSprite(player[1].sprno,100,203,0,11);
			
			// Initialisation du shoot Joueur 1
			shoot[0].sprno = getFreeSprite();
			shoot[1].sprno = getFreeSprite();
			activateSprite(shoot[0].sprno,INVISIBLE,INVISIBLE,24,7);
			activateSprite(shoot[1].sprno,INVISIBLE,INVISIBLE,24,11);
				
			initLevel(level);				
			
			workflow=W_INGAME;
			if (((player[0].nblives)==0) && ((player[1].nblives)==0)) workflow = W_GAMEOVER;
		}
		else
		if (workflow==W_GAMEOVER)
		{
			doScroll=0;
			while (joypad_1 & FIRE1) {}
			while (joypad_1 & FIRE2) {}
			disable_nmi();			
			screen_off();
			cls();
			center_string(4,"GAME OVER");
			center_string(6,"PLAYER 1");
			center_string(7,str(player[0].score));
			
			if (nbPlayers==2)
			{
				center_string(9,"PLAYER 2");
				center_string(10,str(player[1].score));
			}
			
			screen_on();
			enable_nmi();
			pause();			
			workflow=W_SPLASH1;
		}
	}
}

void nmi()
{
			byte i;
			++ticks;
			update_music();
			/******************************/
			/* NOUVEAU SYSTEME FLICKERING */
			/******************************/
			nb_source = 32-p_source;
			memcpyb(bsprites,sprites+p_source,nb_source<<2);
			if (nb_source!=32) memcpyb(bsprites+32-p_source,sprites,(32-nb_source)<<2);			
			put_vram (0x1b00,bsprites,128);				
			p_source +=4; // 4 = A priori valeur optimale
			if (p_source>31) p_source = 0;
			/**********************************/
			/* FIN NOUVEAU SYSTEME FLICKERING */
			/**********************************/
			
			if (doScroll==1)
			{
				scrollVague_index++;
				if (scrollVague_index>15) scrollVague_index = 0;
				put_frame(&allWater+(scrollVague_index<<5),0,11,32,1); // OK
				if (timer8==0) showAndMoveStars();
				if (refresh_score==1)
				{
					refresh_score = 0;
					print_at(4,22,str(player[0].score));
					print_at(10,22,"   ");
					for (i=0;i<player[0].nblives;i++) put_char (10+i,22,'*');
					print_at(23,22,str(player[1].score));
					print_at(19,22,"   ");
					for (i=0;i<player[1].nblives;i++) put_char (19+i,22,'*');
				}					
			}
}