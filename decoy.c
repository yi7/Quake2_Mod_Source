#include "g_local.h"
#include "m_player.h"
#define newDecoy self->decoy

int i;
void SP_Decoy (edict_t *self);
void func_explosive_explode (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

void decoyAI_RunFrames(edict_t *self, int start, int end)
{
	if(self->s.frame <= end)
		self->s.frame++;
	else
		self->s.frame = start;
}

void decoy_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	decoyAI_RunFrames(self, FRAME_pain101, FRAME_pain104);
}
        

void decoy_explode (edict_t *ent)
{               
	vec3_t origin;

	//FIXME: if we are onground then raise our Z just a bit since we are a point?
	T_RadiusDamage(ent, ent->owner, ent->dmg, NULL, ent->dmg_radius,0);
	VectorMA (ent->s.origin, -.02, ent->velocity, origin);
	gi.WriteByte (svc_temp_entity); 
	
	if (ent->waterlevel) {       
		if (ent->groundentity)
			gi.WriteByte (TE_GRENADE_EXPLOSION_WATER);
		else
			gi.WriteByte (TE_ROCKET_EXPLOSION_WATER);
	} else {       
		if (ent->groundentity)
			gi.WriteByte (TE_GRENADE_EXPLOSION);
		else
			gi.WriteByte (TE_ROCKET_EXPLOSION);
	}

	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);           
}

void Decoy_Think (edict_t *ent)
{
	edict_t *blip = NULL;
        
	if (level.time >ent->delay) {
		decoy_explode(ent);
		return;
	}

	ent->think = Decoy_Think;
	while ((blip = findradius (blip, ent->s.origin, 100)) != NULL) {  
		if (!(blip->svflags & SVF_MONSTER) && !blip->client)
			continue;
		if (blip == ent->owner)
			continue;
		if (blip->health <= 0)
			continue;
		if (!visible(ent, blip))
			continue;

		ent->think = decoy_explode;
		break;
	}

	decoyAI_RunFrames(ent,FRAME_stand01,FRAME_stand39);

	ent->nextthink = level.time + .1;
}

void decoy_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int i;
	gi.sound (self, CHAN_BODY, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);

	for(i=0; i<4; i++)
		ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

	self->takedamage = DAMAGE_NO;
       
	gi.WriteByte (svc_temp_entity); 
	gi.WriteByte (TE_BFG_EXPLOSION);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);
    
	self->owner->client->decoynum--;
	gi.centerprintf(self->owner, "decoynum: %d\n",self->owner->client->decoynum);
	//SP_Decoy (self->owner);

	G_FreeEdict(self);
}

void SP_Decoy (edict_t *self)
{
	if(self->client->decoynum) {
		if ( newDecoy && (self->client->decoynum >= 5)) {
			gi.centerprintf (self,"Max 5 Decoys!\n");
        
			return; 
		}
	} else {
		self->client->decoynum = 0;
	}
	
	self->client->decoynum++;
	gi.centerprintf(self, "decoynum: %d\n",self->client->decoynum);

	newDecoy = G_Spawn ();
	VectorCopy(self->s.origin,newDecoy->s.origin);
	VectorCopy(self->s.angles,newDecoy->s.angles);
	newDecoy->classname="decoy";
	newDecoy->takedamage = DAMAGE_AIM;
	newDecoy->movetype = MOVETYPE_TOSS;
	newDecoy->mass = 200;
	newDecoy->solid = SOLID_BBOX;
	newDecoy->deadflag = DEAD_NO;
	newDecoy->clipmask = MASK_PLAYERSOLID;
	newDecoy->model = self->model;
	newDecoy->s.modelindex = self->s.modelindex;
	newDecoy->s.modelindex2 = self->s.modelindex2;
	newDecoy->s.frame = 0;
	newDecoy->s.renderfx |= RF_TRANSLUCENT;
	newDecoy->waterlevel = 0;
	newDecoy->watertype = 0;
	newDecoy->health = 20;
	newDecoy->max_health = 20;
	newDecoy->gib_health = -80;
	newDecoy->pain= decoy_pain;
	newDecoy->think = Decoy_Think;
	newDecoy->nextthink = level.time + .1;
	newDecoy->delay = level.time + 300;
	newDecoy->die = decoy_die;
	newDecoy->owner = self;
	newDecoy->dmg = 100;
	newDecoy->dmg_radius = 100;
               
	VectorSet (newDecoy->mins, -16, -16, -24);
	VectorSet (newDecoy->maxs, 16, 16, 32);
	VectorClear (newDecoy->velocity);
	gi.linkentity (newDecoy);
}