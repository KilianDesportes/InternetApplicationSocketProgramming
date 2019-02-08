#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

void
construire_message (char *message, char motif, int lg)
{
  int i;
  for (i = 0; i < lg; i++)
    {
      message[i] = motif;
    }
}

void
afficher_message (char *message, int lg)
{
  int i;
  printf ("[");
  for (i = 0; i < lg; i++)
    {
      printf ("%c", message[i]);
    }
  printf ("]\n");
}

int
main (int argc, char **argv)
{
  int c;
  extern char *optarg;
  extern int optind;
  int nb_message = 10;          /* Nb de messages à envoyer ou à recevoir, par défaut : 10 en émission, infini en réception */
  int source = -1;              /* 0 = Serveur, 1 = Client */
  int type_client = -1;         /* 0 = Recepteur , 1 = Emetteur */
  int len_message = 30;         /* Longueur du message, defaut : 30 */
  int nb_msg_modif = 0;         /* 1 = modification, 0 = pas de modification */
  int nb_port = -1;             /* Numéro de port */
  int protocole = 0;            /* 0 = TCP, 1 = UDP */

  int nb_port_htons = -1;       /* Numéro de port big endian */
  char *host_name;              /* Nom d'hôte */
  int sock;                     /* Socket */
  char *proc;                   /* Nom du protocole */
  // Partie INITIALISATION DES PARAMETRES
  while ((c = getopt (argc, argv, "creusl:n:")) != -1)
    {
      switch (c)
        {
        case 's':
          if (source == 1)
            {
              printf
                ("usage: tsock [-c [-e|-r]|-s] [-n ##] [-l ##] [host] port\n");
              exit (1);
            }
          source = 0;
          break;
        case 'c':
          if (source == 0)
            {
              printf
                ("usage: tsock [-c [-e|-r]|-s] [-n ##] [-l ##] [host] port\n");
              exit (1);
            }
          source = 1;
          host_name = argv[argc - 2];
          break;
        case 'r':
          type_client = 0;
          break;
        case 'e':
          type_client = 1;
          break;
        case 'l':
          len_message = atoi (optarg);
          break;
        case 'u':
          protocole = 1;
          break;
        case 'n':
          nb_message = atoi (optarg);
          nb_msg_modif = 1;
          break;
        default:
          printf
            ("usage: tsock [-c [-e|-r]|-s] [-n ##] [-l ##] [host] port\n");
          break;
        }
    }
  if (source == -1)
    {
      printf ("usage: tsock [-c [-e|-r]|-s] [-n ##] [-l ##] [host] port\n");
      exit (1);
    }
  nb_port = atoi (argv[argc - 1]);
  nb_port_htons = htons (nb_port);
  if (nb_port == -1)
    {
      printf ("usage: tsock [-c [-e|-r]|-s] [-n ##] [-l ##] [host] port\n");
      exit (1);
    }

  // Partie EMISSION DU MESSAGE
  if (protocole == 1)
    {
      if ((sock = socket (AF_INET, SOCK_DGRAM, 0)) == -1)
        {
          perror ("Echec de création du socket\n");
          exit (1);
        }
      else
        {
          proc = "UDP";
        }
    }
  else
    {
      if ((sock = socket (AF_INET, SOCK_STREAM, 0)) == -1)
        {
          perror ("Echec de création du socket\n");
          exit (1);
        }
      else
        {
          proc = "TCP";
        }
    }

  // SERVEUR
  if (protocole == 0)
    {
      //PROTOCOLE TCP
      if (source == 0)
        {
          struct sockaddr_in adr_local;
          memset ((char *) &adr_local, 0, sizeof (adr_local));
          adr_local.sin_family = AF_INET;
          adr_local.sin_port = nb_port;
          adr_local.sin_addr.s_addr = INADDR_ANY;
          int lg_adr_local = sizeof (adr_local);
          int option = 1;
          if (setsockopt
              (sock, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR),
               (char *) &option, sizeof (option)) < 0)
            {
              printf ("setsockopt failed\n");
              close (sock);
              exit (1);
            }
          if (bind (sock, (struct sockaddr *) &adr_local, lg_adr_local) == -1)
            {
              perror ("Echec du bind\n");
              exit (1);
            }
          char msg[len_message];
          int i = 1;
          int nb_lu;
          listen (sock, 10);
          int sock_bis;
          while (1)
            {
              if ((sock_bis =
                   accept (sock, (struct sockaddr *) &adr_local,
                           &lg_adr_local)) == -1)
                {
                  perror ("Echec accept");
                  exit (1);
                }
              switch (fork ())
                {
                case -1:
                  perror ("Echec fork");
                  exit (1);
                case 0:
                  close (sock);
                  if (type_client == 1)
                    {
                      char *msg = malloc (sizeof (char) * len_message);
                      printf
                        ("SOURCE : port=%d, nb_reception=infini, nb_envois=%d, TP=%s\n",
                         nb_port, nb_message, proc);
                      for (i = 0; i < nb_message; i++)
                        {
                          construire_message (msg, 'a', len_message);
                          int ret = write (sock_bis, msg, len_message);
                          printf ("SOURCE : Envoi n°%d (%d) ", (i + 1),
                                  len_message);
                          afficher_message (msg, len_message);
                        }
                      printf ("SOURCE : Fin\n");
                    }
                  else
                    {
                      printf
                        ("PUITS : lg_mesg-lu=%d, port=%d, nb_reception=infini, TP=%s socket n°%d\n",
                         len_message, nb_port, proc, sock_bis);
                      if (nb_msg_modif == 1)
                        {
                          int j;
                          for (j = 0; j < nb_message; j++)
                            {
                              if (nb_lu = read (sock_bis, msg, len_message))
                                {
                                  printf ("PUITS : Reception n°%d (%d) ", i,
                                          nb_lu);
                                  afficher_message (msg, nb_lu);
                                }
                            }
                        }
                      else
                        {
                          while ((nb_lu =
                                  read (sock_bis, msg, len_message)) > 0)
                            {
                              printf ("PUITS : Reception n°%d (%d) ", i,
                                      nb_lu);
                              afficher_message (msg, nb_lu);
                              i++;
                            }
                        }
                    }
                  if (close (sock_bis) == -1)
                    {
                      perror ("Echec destruction du socket source");
                    }
                  else
                    {
                      printf ("-- Destruction socket %d --\n", sock_bis);

                    }
                  exit (0);
                default:
                  close (sock_bis);
                }
            }
        }
      else if (type_client == 0)
        {
          //CLIENT RECEPTEUR
          struct hostent *hp;
          struct sockaddr_in adr_local;
          memset ((char *) &adr_local, 0, sizeof (adr_local));
          adr_local.sin_family = AF_INET;
          adr_local.sin_port = nb_port;
          if ((hp = gethostbyname (host_name)) == NULL)
            {
              perror ("Erreur gethostbyname");
              exit (1);
            }
          memcpy ((char *) &(adr_local.sin_addr.s_addr),
                  hp->h_addr, hp->h_length);
          int lg_adr_local = sizeof (adr_local);
          int nb_char_sent, nb_lu;
          int i = 0;
          char *msg = malloc (sizeof (char) * len_message);
          if (connect (sock, (struct sockaddr *) &adr_local, lg_adr_local) !=
              0)
            {
              perror ("Erreur connect");
              exit (1);
            }
          else
            {
              printf
                ("PUITS : lg_mesg-lu=%d, port=%d, nb_reception=infini, TP=%s socket n°%d\n",
                 len_message, nb_port, proc, sock);
              if (nb_msg_modif == 1)
                {
                  int j;
                  for (j = 0; j < nb_message; j++)
                    {
                      if (nb_lu = read (sock, msg, len_message))
                        {
                          printf ("PUITS : Reception n°%d (%d) ", (i + 1),
                                  nb_lu);
                          afficher_message (msg, nb_lu);
                          i++;
                        }
                    }
                }
              else
                {
                  while ((nb_lu = read (sock, msg, len_message)) > 0)
                    {
                      printf ("PUITS : Reception n°%d (%d) ", (i + 1),
                              nb_lu);
                      afficher_message (msg, nb_lu);
                      i++;
                    }
                }
              if (close (sock) == -1)
                {
                  perror ("Echec destruction du socket source");
                }
              else
                {
                  printf ("-- Destruction socket %d --\n", sock);
                }
            }

        }
      else
        {
          //CLIENT EMETTEUR
          struct hostent *hp;
          struct sockaddr_in adr_distant;
          memset ((char *) &adr_distant, 0, sizeof (adr_distant));
          adr_distant.sin_family = AF_INET;
          adr_distant.sin_port = nb_port;
          if ((hp = gethostbyname (host_name)) == NULL)
            {
              perror ("Erreur gethostbyname");
              exit (1);
            }
          memcpy ((char *) &(adr_distant.sin_addr.s_addr),
                  hp->h_addr, hp->h_length);
          int lg_adr_distant = sizeof (adr_distant);
          int nb_char_sent, i;
          if (connect (sock, (struct sockaddr *) &adr_distant, lg_adr_distant)
              != 0)
            {
              perror ("Erreur connect");
              exit (1);
            }
          else
            {
              char *msg = malloc (sizeof (char) * len_message);
              printf
                ("SOURCE : port=%d, nb_reception=infini, nb_envois=%d, TP=%s, dest=%s\n",
                 nb_port, nb_message, proc, host_name);
              for (i = 0; i < nb_message; i++)
                {
                  construire_message (msg, 'a', len_message);
                  int ret = write (sock, msg, len_message);
                  printf ("SOURCE : Envoi n°%d (%d) ", (i + 1), len_message);
                  afficher_message (msg, len_message);
                }
              printf ("SOURCE : Fin\n");
            }
        }
    }
  else
    {
      //PROTOCOLE UDP
      if (source == 0)
        {
          struct sockaddr_in adr_local;
          memset ((char *) &adr_local, 0, sizeof (adr_local));
          adr_local.sin_family = AF_INET;
          adr_local.sin_port = nb_port;
          adr_local.sin_addr.s_addr = INADDR_ANY;
          int lg_adr_local = sizeof (adr_local);
          int option = 1;
          if (setsockopt
              (sock, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR),
               (char *) &option, sizeof (option)) < 0)
            {
              printf ("setsockopt failed\n");
              close (sock);
              exit (1);
            }
          if (bind (sock, (struct sockaddr *) &adr_local, lg_adr_local) == -1)
            {
              perror ("Echec du bind\n");
              exit (1);
            }
          char msg[len_message];
          int i = 1;
          int nb_lu;
          listen (sock, 10);
          int sock_bis;
          while (1)
            {
              if ((sock_bis =
                   accept (sock, (struct sockaddr *) &adr_local,
                           &lg_adr_local)) == -1)
                {
                  perror ("Echec accept");
                  exit (1);
                }
              switch (fork ())
                {
                case -1:
                  perror ("Echec fork");
                  exit (1);
                case 0:
                  close (sock);
                  if (type_client == 1)
                    {
                      char *msg = malloc (sizeof (char) * len_message);
                      printf
                        ("SOURCE : port=%d, nb_reception=infini, nb_envois=%d, TP=%s\n",
                         nb_port, nb_message, proc);
                      for (i = 0; i < nb_message; i++)
                        {
                          construire_message (msg, 'a', len_message);
                          int ret = write (sock_bis, msg, len_message);
                          printf ("SOURCE : Envoi n°%d (%d) ", (i + 1),
                                  len_message);
                          afficher_message (msg, len_message);
                        }
                      printf ("SOURCE : Fin\n");
                    }
                  else
                    {
                      printf
                        ("PUITS : lg_mesg-lu=%d, port=%d, nb_reception=infini, TP=%s socket n°%d\n",
                         len_message, nb_port, proc, sock_bis);
                      if (nb_msg_modif == 1)
                        {
                          int j;
                          for (j = 0; j < nb_message; j++)
                            {
                              if (nb_lu = read (sock_bis, msg, len_message))
                                {
                                  printf ("PUITS : Reception n°%d (%d) ", i,
                                          nb_lu);
                                  afficher_message (msg, nb_lu);
                                }
                            }
                        }
                      else
                        {
                          while ((nb_lu =
                                  read (sock_bis, msg, len_message)) > 0)
                            {
                              printf ("PUITS : Reception n°%d (%d) ", i,
                                      nb_lu);
                              afficher_message (msg, nb_lu);
                              i++;
                            }
                        }
                    }
                  if (close (sock_bis) == -1)
                    {
                      perror ("Echec destruction du socket source");
                    }
                  else
                    {
                      printf ("-- Destruction socket %d --\n", sock_bis);

                    }
                  exit (0);
                default:
                  close (sock_bis);
                }
            }
        }
      else if (type_client == 0)
        {
          //CLIENT RECEPTEUR
          struct hostent *hp;
          struct sockaddr_in adr_local;
          memset ((char *) &adr_local, 0, sizeof (adr_local));
          adr_local.sin_family = AF_INET;
          adr_local.sin_port = nb_port;
          if ((hp = gethostbyname (host_name)) == NULL)
            {
              perror ("Erreur gethostbyname");
              exit (1);
            }
          memcpy ((char *) &(adr_local.sin_addr.s_addr),
                  hp->h_addr, hp->h_length);
          int lg_adr_local = sizeof (adr_local);
          int nb_char_sent, nb_lu;
          int i = 0;
          char *msg = malloc (sizeof (char) * len_message);
          if (connect (sock, (struct sockaddr *) &adr_local, lg_adr_local) !=
              0)
            {
              perror ("Erreur connect");
              exit (1);
            }
          else
            {
              printf
                ("PUITS : lg_mesg-lu=%d, port=%d, nb_reception=infini, TP=%s socket n°%d\n",
                 len_message, nb_port, proc, sock);
              if (nb_msg_modif == 1)
                {
                  int j;
                  for (j = 0; j < nb_message; j++)
                    {
                      if (nb_lu = read (sock, msg, len_message))
                        {
                          printf ("PUITS : Reception n°%d (%d) ", (i + 1),
                                  nb_lu);
                          afficher_message (msg, nb_lu);
                          i++;
                        }
                    }
                }
              else
                {
                  while ((nb_lu = read (sock, msg, len_message)) > 0)
                    {
                      printf ("PUITS : Reception n°%d (%d) ", (i + 1),
                              nb_lu);
                      afficher_message (msg, nb_lu);
                      i++;
                    }
                }
              if (close (sock) == -1)
                {
                  perror ("Echec destruction du socket source");
                }
              else
                {
                  printf ("-- Destruction socket %d --\n", sock);
                }
            }

        }
      else
        {
          //CLIENT EMETTEUR
          struct hostent *hp;
          struct sockaddr_in adr_distant;
          memset ((char *) &adr_distant, 0, sizeof (adr_distant));
          adr_distant.sin_family = AF_INET;
          adr_distant.sin_port = nb_port;
          if ((hp = gethostbyname (host_name)) == NULL)
            {
              perror ("Erreur gethostbyname");
              exit (1);
            }
          memcpy ((char *) &(adr_distant.sin_addr.s_addr),
                  hp->h_addr, hp->h_length);
          int lg_adr_distant = sizeof (adr_distant);
          int nb_char_sent, i;
          if (connect (sock, (struct sockaddr *) &adr_distant, lg_adr_distant)
              != 0)
            {
              perror ("Erreur connect");
              exit (1);
            }
          else
            {
              char *msg = malloc (sizeof (char) * len_message);
              printf
                ("SOURCE : port=%d, nb_reception=infini, nb_envois=%d, TP=%s, dest=%s\n",
                 nb_port, nb_message, proc, host_name);
              for (i = 0; i < nb_message; i++)
                {
                  construire_message (msg, 'a', len_message);
                  int ret = write (sock, msg, len_message);
                  printf ("SOURCE : Envoi n°%d (%d) ", (i + 1), len_message);
                  afficher_message (msg, len_message);
                }
              printf ("SOURCE : Fin\n");
            }
        }
    }
  return 0;
}
