#ifndef ACTIONS_H_
#define ACTIONS_H_
#include "Vector.h"
#include "Arduino.h"
#include "MoveProfile.h"
#include "Communication.h"
#include "SequenceName.h"
#include "ErrorManager.h"
#include <vector>
#include <cstdint> //for macro INT16_MAX

class Robot;
class Sequence;
typedef void (*Fct)(Robot *robot);

//========================================ACTION GENERIQUES========================================
#define NO_REQUIREMENT INT16_MAX

/*
* CLASSE ABSTRAITE. NE PAS INSTANCIER DIRECTEMENT
*/
class Action
{
public:
    /*
    * Le nom de l'action
    */
    String name;

    /*
    * La fonction start est appelé une fois au début de l'action si l'action désignée par le "requirement" a réussi
    * Sinon c'est un fail immédiat.
    * Si l'action est interrompu par Sequence::pause, le start sera rappelé lors du resume
    */
    virtual void start();

    /*
    * IsFinished est appelé en boucle. Il renvoie si l'action est terminé
    * Peut aussi servir de fonction d'update...
    */
    virtual bool isFinished() { return done; }

    /*
    * HasFailed est appelé en boucle. Il renvoie si l'action a foiré et devrait etre laissé de coté
    * La Sequence retiendra cepenant que cette fonction a foiré (utile pour les requirements)
    * Par défaut, une action foire si elle dépasse le temps qui lui a été imparti par le timeout.
    * (Un timeout de -indique que l'Action dispose d'un temps illimité)
    */
    virtual bool hasFailed();

    /*
    * Passe à toutes les actions courantes et a venir un pointeur vers le robot
    */
    static void setPointer(Robot *robot);

    /*
    * Cree une action de base
    * Cette classe est abstraite et ne dois pas être instanciée directement
    */
    Action(String name = "Action", float timeout = 0.1, int16_t require = NO_REQUIREMENT)
    {
        this->name = name;
        this->timeout = timeout;
        this->require = require;
        done = false;
        started = false;
    }

    /*
    * Renvoie si l'action en cours a été started ou non
    */
    bool hasStarted() { return started; }

    /*
    * Cette fonction est appelée en cas de réussite de l'action. Ne fait rien par défaut
    */
    virtual void doAtEnd()
    { /*Ne fait rien par défaut. Il faudra override plus tard*/
    }

protected:
    bool done;
    bool started;
    float timeout;
    float timeStarted;
    static Robot *robot;
    Sequence *mySequence;
    int16_t require;

    friend class Sequence;
};

/*
* Une doube action est une séquence de deux actions.
* La gestion de ces deux actions est encapsulée. Il n'y a rien a faire de particulier
* Une double action est terminée si les deux actions sont terminées
* Une double actin foire si l'une des actions foire
*/
class Double_Action : public Action
{
protected:
    Action *action1;
    Action *action2;

public:
    virtual void start();
    virtual bool isFinished();
    virtual bool hasFailed();
    void doAtEnd() override;
    Double_Action(float timeout, String name = "Twin", int16_t require = NO_REQUIREMENT);
};

//========================================ACTION MOVES========================================

/*
* Une Move Action est une action qui va donner un nouvel ordre au ghost
* Lors de l'appel de "start", la position cible est donnée au ghost qui va ensuite s'y rendre (le ghoost est alors delock si necessaire)
* Par défaut, les PID sont reset (Iterm) à la fin de l'action
*/
class Move_Action : public Action //Classe abstraite
{
public:
    virtual void start();      //(Action+Move)Dump les parametres dans le ghost et appelle Action::start() et debloque le ghost
    virtual bool isFinished(); //(Move) Verifie que le ghost est arrive et que le robot est sur le ghost
    virtual bool hasFailed();  //(Action+Move) Verifie que le pid n'a pas retourné d'erreur ou que Action::hasFailed n'est pas true
    void doAtEnd() override;
    Move_Action(float timeout, VectorE posFinal, float deltaCurve,
                MoveProfileName profileName, bool pureRotation, bool backward, String name = "Move", int16_t require = NO_REQUIREMENT);

protected:
    VectorE posFinal;
    float deltaCurve;
    MoveProfileName profileName;
    bool pureRotation, backward;
};

/*
* Va a la position demandée (x,y,theta) avec une courbure deltaCurve et un rythme pace. (peut etre effectue en marche arriere)
* /!\ COLOR DEPENDANT
*/
class Goto_Action : public Move_Action
{
public:
    Goto_Action(float timeout, TargetVectorE target, float deltaCurve, MoveProfileName profileName, bool backward = false, int16_t require = NO_REQUIREMENT);
    //start (Action+Move)
    //isFinished (Move)
    //hasFailed (Action+Move)
};

/*
* Tourne sur place pour rejoindre la position demandée
* /!\ COLOR DEPENDANT
*/
class Spin_Action : public Move_Action
{
public:
    Spin_Action(float timeout, TargetVectorE target, MoveProfileName profileName, int16_t require = NO_REQUIREMENT);
    void start(); //(Action + Spin) Le start doit etre redéfini car on ne connait pas posFinal a l'avance
    //isFinished(Move)
    //hasFailed(Action+Move)
};

/*
* Tourne sur place d'un certain angle deltaTheta (peut importe la couleur)
*/
class Rotate_Action : public Move_Action //Tourne en relatif
{
private:
    float deltaTheta;

public:
    Rotate_Action(float timeout, float deltaTheta, MoveProfileName profileName, int16_t require = NO_REQUIREMENT);
    void start(); //(Action + Spin) Le start doit etre redéfini car on ne connait pas posFinal a l'avance
    //isFinished(Move)
    //hasFailed(Action+Move)
};

/*
* Avance tout droit d'une certaine distance dist (peut importe la couleur)
*/
class Forward_Action : public Move_Action
{
private:
    float dist;

public:
    Forward_Action(float timeout, float dist, MoveProfileName profileName, int16_t require = NO_REQUIREMENT);
    void start(); //(Action + Forward)Le start doit etre redéfini car on ne connait pas posFinal a l'avance
    //isFinished(Move)
    //hasFailed(Action+Move)
};

class Backward_Action : public Move_Action
{
private:
    float dist;

public:
    Backward_Action(float timeout, float dist, MoveProfileName profileName, int16_t require = NO_REQUIREMENT);
    void start(); //(Action + Backward)Le start doit etre redéfini car on ne connait pas posFinal a l'avance
    bool isFinished();
    bool hasFailed();
    //hasFailed(Action+Move)
};

/*
* Rejoins la target en faisant d'abord un spin puis une ligne droite.
* /!\ COLOR DEPENDANT
*/
class StraightTo_Action : public Double_Action
{
private:
    Spin_Action *spin;
    Goto_Action *goTo;
    float x, y;
    MoveProfileName profileName;
    float timeout;

public:
    void start();
    StraightTo_Action(float timeout, TargetVector target, MoveProfileName profileName, int16_t require = NO_REQUIREMENT);
};

/*
* Freine le robot jusqu'a ce que ça vitesse (angulaire) passe sous le dEpsilon donné dans le brake profile
* cf MoveProfile.cpp -> setup
*/
class Brake_Action : public Move_Action
{
public:
    Brake_Action(float timeout, int16_t require = NO_REQUIREMENT);
    void start();
};

//========================================ACTION COMM========================================

/*
* Envoie un message sur le port de communication
*/
class Send_Action : public Action
{
private:
    Message message;
    Communication* _commLocal;

public:
    Send_Action(Message message, Communication* comm, int16_t require = NO_REQUIREMENT);
    void start(); //(Action+Send)
    //isFinished(Action)
    //hasFailed(Action)
};

/*
* Instruction bloquante: Attend un message sur le port de communication
*/
class Wait_Message_Action : public Action
{
private:
    MessageID messageId;
    Communication* _commLocal;

public:
    Wait_Message_Action(MessageID messageId, float timeout, Communication* comm, int16_t require = NO_REQUIREMENT);
    //start(Action)
    bool isFinished(); //(Wait_Message) verifie que le message est recu
    //hasFailed(Action)
};

/*
* Permet d'assigner une Function par message possible
*/
class Switch_Message_Action : public Action
{
private:
    std::vector<MessageID> onMessage;
    std::vector<Fct> doFct;
    uint8_t size;
    Communication* _commLocal;

public:
    Switch_Message_Action(float timeout, Communication* comm, int16_t require);
    void addPair(MessageID messageId, Fct fct);
    //start : inherited from Action
    bool isFinished();
    //has failed : inherited from Action
};

/*
* Envoie un ordre d'action vers la carte actionneur.
* waitCompletion : Attente ou non de l'execution de l'action par la carte actionneur.
*/
class Send_Order_Action : public Double_Action
{
private:
    Send_Action* sendAction;
    Wait_Message_Action* waitAction;

    Message message;
public:
    Send_Order_Action(MessageID actuatorID, Actuator_Order actuatorOrder, float timeout, Communication *comm, boolean waitCompletion, int16_t require = NO_REQUIREMENT);
};

//========================================ACTION MISC========================================

/*
* Ne fais rien pendant un certain temps.
* Le ghost continue sur sa dernière action
*/
class Sleep_Action : public Action
{
private:
    float timeToWait;

public:
    Sleep_Action(float timeToWait, int16_t require = NO_REQUIREMENT);
    //start(Action)
    bool isFinished();                 //(Sleep) verifie que le temps prévu s'est ecoulé
    bool hasFailed() { return false; } //(Sleep) on en peut pas fail d'attendre
};

/*
* Ne fais rien.
* Permet d'annuler une action d'un Double_Action.
*/
class Null_Action : public Action
{
public:
    Null_Action();
    bool isFinished() {return true;}
    bool hasFailed() {return false;}
};

/*
* Ne fait rien et est impossible a passer.
* Si loop est activé, cette action permet de retourner a la première action de la file
* Si pause est activé, la séquence ne s'actualise plus tant qu'elle n'est pas resume
* Si lockGhost est activé, le ghost ne s'actualise plus
* Par défaut, une endAction endort une sequence
*/
class End_Action : public Action //Une End_Action ne passe jamais a la suite
{
private:
    bool loop;
    bool pause;
    bool lockGhost;

public:
    End_Action(bool loop = false, bool pause = true, bool lockGhost = false);
    void start();
    bool isFinished() { return done; }
    bool hasFailed() { return false; }
};

/*
* Fait l'action "functionToCall" lors du start de l'action
*/
class Do_Action : public Action
{
private:
    Fct functionToCall;

public:
    void start();
    Do_Action(Fct functionToCall, int16_t require = NO_REQUIREMENT) : Action("DoAc", 0.1, require) { this->functionToCall = functionToCall; }
};

/*
* Met en pause une sequence lors de start
*/
class PauseSeq_Action : public Action
{
private:
    SequenceName nameSeq;
    bool lockGhost;

public:
    void start();
    PauseSeq_Action(SequenceName nameSeq, bool lockGhost, int16_t require = NO_REQUIREMENT);
};

/*
* Relance une sequence lors de start
*/
class ResumeSeq_Action : public Action
{
private:
    SequenceName nameSeq;

public:
    void start();
    ResumeSeq_Action(SequenceName nameSeq, int16_t require = NO_REQUIREMENT);
};

/*
* Attend une erreur
*/
class Wait_Error_Action : public Action
{
private:
    Error error;

public:
    Wait_Error_Action(Error error, float timeout, int16_t require = NO_REQUIREMENT);
    //void start();
    bool isFinished(); //(Wait_Error) verifie que l'erreur s'est produite
    //hasFailed(Action)
};

class Recallage_Action : public Double_Action
{
public:
    Recallage_Action(bool arriere, float dist, float timeout);
};

//========================================ACTION INPUT========================================

/*
* Attend que la tirette soit tiree
* Le pin est Low lorsque la tirette est branche
*/
class Wait_Tirette_Action : public Action
{
private:
    uint8_t pinIN;
    bool initOK = false;

public:
    Wait_Tirette_Action(uint8_t pinIN, int16_t require = NO_REQUIREMENT);
    void start();
    bool isFinished();
    bool hasFailed() { return false; } // On ne peut pas fail l'attente
};

#endif // !ACTION_H_