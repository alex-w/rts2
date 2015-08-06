#include "apm-relay.h"
#include "connection/apm.h"

using namespace rts2sensord;

APMRelay::APMRelay (int argc, char **argv, const char *sn, rts2filterd::APMFilter *in_filter): Sensor (argc, argv, sn)
{
        addOption ('e', NULL, 1, "IP and port (separated by :)");
        addOption ('F', NULL, 1, "filter names, separated by : (double colon)");

        createValue(relay1, "relay1", "Relay 1 state", true);
        createValue(relay2, "relay2", "Relay 2 state", true);

        filter = in_filter;
}

int APMRelay::initHardware ()
{
        connRelay = filter->connFilter;

        sendUDPMessage ("A999");

        return 0;
}

int APMRelay::commandAuthorized (rts2core::Connection * conn)
{
        if (conn->isCommand ("on"))
        {
                int n;
                if (conn->paramNextInteger (&n) || !conn->paramEnd ())
                        return -2;
                return relayOn (n);
        }

        if (conn->isCommand ("off"))
        {
                int n;
                if (conn->paramNextInteger (&n) || !conn->paramEnd ())
                        return -2;
                return relayOff (n);
        }

        return 0;
}

int APMRelay::info ()
{
        sendUDPMessage ("A999");

        switch (relay1_state)
        {
                case RELAY1_ON:
                        relay1->setValueString("on");
                        break;
                case RELAY1_OFF:
                        relay1->setValueString("off");
                        break;
        }

        switch (relay2_state)
        {
                case RELAY2_ON:
                        relay2->setValueString("on");
                        break;
                case RELAY2_OFF:
                        relay2->setValueString("off");
                        break;
        }

        sendValueAll(relay1);
        sendValueAll(relay2);

        return Sensor::info();
}

int APMRelay::processOption (int in_opt)
{
        switch (in_opt)
        {
                case 'e':
                        return 0;
                case 'F':
                        return 0;
                default:
                        return Sensor::processOption (in_opt);
        }
}

int APMRelay::relayOn (int n)
{
        char *cmd = (char *)malloc(4*sizeof (char));

        sprintf (cmd, "A0%d1", n);

        sendUDPMessage (cmd);

        info ();

        return 0;
}

int APMRelay::relayOff (int n)
{
        char *cmd = (char *)malloc(4*sizeof (char));

        sprintf (cmd, "A0%d0", n);

        sendUDPMessage (cmd);

        info ();

        return 0;
}

int APMRelay::sendUDPMessage (const char * _message)
{
        char *response = (char *)malloc(20*sizeof(char));

        logStream (MESSAGE_DEBUG) << "command: " << _message << sendLog;

        int n = connRelay->send (_message, response, 20);

        logStream (MESSAGE_DEBUG) << "response: " << response << sendLog;

	if (n > 0)
        {
                if (response[0] == 'A' && response[1] == '9')
                {
                        if (response[3] == '3')
                        {
                                relay1_state = RELAY1_ON;
                                relay2_state = RELAY2_ON;
                        }

                        if (response[3] == '2')
                        {
                                relay1_state = RELAY1_OFF;
                                relay2_state = RELAY2_ON;
                        }

                        if (response[3] == '1')
                        {
                                relay1_state = RELAY1_ON;
                                relay2_state = RELAY2_OFF;
                        }

			if (response[3] == '0')
                        {
                                relay1_state = RELAY1_OFF;
                                relay2_state = RELAY2_OFF;
                        }
                }
        }

        return 0;
}
