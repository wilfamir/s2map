/*
  A trivial static http webserver using Libevent's evhttp.

  This is not the best code in the world, and it does some fairly stupid stuff
  that you would never want to do in a production webserver. Caveat hackor!

 */
#include <curl/curl.h>
#include <curl/easy.h>

#include <boost/scoped_ptr.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>

#include "s2cellid.h"
#include "s2cell.h"
#include "s2.h"
#include "s2polygon.h"
#include "s2polygonbuilder.h"
#include "s2latlng.h"
#include "s2regioncoverer.h"

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#ifndef S_ISDIR
#define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif
#else
#include <sys/stat.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#endif

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#ifdef _EVENT_HAVE_NETINET_IN_H
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#endif

/* Compatibility for possible missing IPv6 declarations */
#include "../util-internal.h"

#ifdef WIN32
#define stat _stat
#define fstat _fstat
#define open _open
#define close _close
#define O_RDONLY _O_RDONLY
#endif

char uri_root[512];

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}

#include <errno.h>

void s2cellidToJson(S2CellId* s2cellid, std::ostringstream& stringStream, bool last) {
  S2Cell cell(*s2cellid);
  S2LatLng center(cell.id().ToPoint());
  stringStream.precision(30);

  stringStream << "{" << endl
    << "\"id\": \"" << cell.id().id() << "\","  << endl
    << "\"id_signed\": \"" << (long long)cell.id().id() << "\","  << endl
    << "\"token\": \"" << cell.id().ToToken() << "\"," << endl
    << "\"pos\":" << cell.id().pos() << ","  << endl
    << "\"face\":" << cell.id().face() << ","  << endl
    << "\"level\":" << cell.id().level() << ","  << endl
      << "\"ll\": { " << endl
        << "\"lat\":" << center.lat().degrees() << ","  << endl
        << "\"lng\":" << center.lng().degrees() << "" << endl
     << "}," << endl
     << "\"shape\": [ " << endl;

   for (int i = 0; i < 4; i++) {
     S2LatLng vertex(cell.GetVertex(i));
     stringStream << "{ " << endl
        << "\"lat\":" << vertex.lat().degrees() << ","  << endl
        << "\"lng\":" << vertex.lng().degrees() << "" << endl
        << "}" << endl;
      if (i != 3) {
        stringStream << ",";
      }
    }

   stringStream
     << "]" << endl
    << "}";

    if (!last) {
      stringStream << ",";
    }
    
    stringStream << endl;
}


// TODO
// get it outtputting json for s2info
// make sure everything is in a scoped_ptr
// s2 covering output

static char *s2CellIdsToJson(char* callback, std::vector<S2CellId> ids)  {
  std::ostringstream stringStream;
  if (callback) {
    stringStream << callback << "(";
  }

  stringStream << "[";

  for (int i = 0; i < ids.size(); i++) {
    s2cellidToJson(&ids[i], stringStream, i == ids.size() - 1);
  }
    
  stringStream << "]";

  if (callback) {
    stringStream << ")";
  }
  
  return strdup(stringStream.str().c_str());
}

static void
s2cover_request_cb(struct evhttp_request *req, void *arg)
{
  struct evkeyvalq  args;

  struct evbuffer *inputBuffer = evhttp_request_get_input_buffer (req);
  size_t record_len = evbuffer_get_length(inputBuffer);
  char *postData = NULL;
  const char *path = "/?";
  if (record_len > 0) { 
    postData = (char *)malloc(strlen(path) + record_len + 10);
    postData[0] = '/';
    postData[1] = '?';
    evbuffer_pullup(inputBuffer, -1);                                                                                                                                                     
    evbuffer_copyout(inputBuffer, (char *)postData + strlen(path), record_len);
    postData[strlen(path) + record_len] = '\0';
    printf("trying to parse: %s\n", (const char *)postData);
    evhttp_parse_query((const char *)postData, &args);
    char* points = (char *)evhttp_find_header(&args, "points");
    cout << points << endl;
  } else {
    const char *uri = evhttp_request_get_uri(req);
    evhttp_parse_query(uri, &args);
  }

  char* callback = (char *)evhttp_find_header(&args, "callback");

  char* points = (char *)evhttp_find_header(&args, "points");
  std::vector<S2CellId> cellids_vector;
  if (points != NULL) {
    printf(points);
    scoped_ptr<S2PolygonBuilder> builder(new S2PolygonBuilder(S2PolygonBuilderOptions::DIRECTED_XOR()));
    
    std::vector<std::string> points_vector = split(string(points), ',');
    std::vector<S2Point> s2points_vector;

    for (int i = 0; i < points_vector.size(); i += 2) {
      char *endptr;
      s2points_vector.push_back(S2LatLng::FromDegrees(
        strtod(points_vector[i].c_str(), &endptr),
        strtod(points_vector[i+1].c_str(), &endptr)
      ).ToPoint());
    }

    int min_level_int = NULL;
    char* min_level = (char *)evhttp_find_header(&args, "min_level");
    if (min_level) {
      min_level_int = atoi(min_level);
      if (min_level_int > S2::kMaxCellLevel) {
        min_level_int = S2::kMaxCellLevel;
      }
      if (min_level_int < 0) {
        min_level_int = 0;
      }
    }

    int max_level_int = NULL;
    char* max_level = (char *)evhttp_find_header(&args, "max_level");
    if (max_level) {
      max_level_int = atoi(max_level);
      if (max_level_int > S2::kMaxCellLevel) {
        max_level_int = S2::kMaxCellLevel;
      }
      if (max_level_int < 0) {
        max_level_int = 0;
      }
    }
      
    char* level_mod = (char *)evhttp_find_header(&args, "level_mod");
    int level_mod_int = 1;
    if (level_mod) {
      level_mod_int = atoi(level_mod);
    }

    printf("%d\n", s2points_vector.size());
    if (s2points_vector.size() == 1) {
      if (max_level_int == NULL) {
        max_level_int = min_level_int;
      }

      for (int i = min_level_int; i <= max_level_int; i += level_mod_int) {
        cellids_vector.push_back(S2CellId::FromPoint(s2points_vector[0]).parent(i));
      }
    } else {
      for (int i = 0; i < s2points_vector.size(); i++) {
        builder->AddEdge(
          s2points_vector[i],
          s2points_vector[(i + 1) % s2points_vector.size()]);
      }

      S2Polygon polygon;
      typedef vector<pair<S2Point, S2Point> > EdgeList;
      EdgeList edgeList;
      builder->AssemblePolygon(&polygon, &edgeList);

      S2RegionCoverer coverer;

      if (min_level_int != NULL) {
        coverer.set_min_level(min_level_int);
      }
      
      if (max_level_int != NULL) {
        coverer.set_max_level(max_level_int);
      }

      if (level_mod_int) {
        coverer.set_level_mod(level_mod_int);
      }

      char* max_cells = (char *)evhttp_find_header(&args, "max_cells");
      if (max_cells) {
        coverer.set_max_cells(atoi(max_cells));
      }

      coverer.GetCovering(polygon, &cellids_vector); 
    }
  }

  printf("\n");

	evhttp_add_header(evhttp_request_get_output_headers(req),
		    "Content-Type", "application/json");
	struct evbuffer *evb = NULL;
	evb = evbuffer_new();
  char* json = s2CellIdsToJson(callback, cellids_vector);
  evbuffer_add_printf(evb, "%s", json);
	evhttp_send_reply(req, 200, "OK", evb);
 
  free(json);

  if (postData)
    free(postData);

  if (evb)
    evbuffer_free(evb);
}

size_t write_data(void *ptr, size_t size, size_t nmemb, struct evbuffer *evb) {
  size_t toWrite = size * nmemb;
  cout << ptr;
  evbuffer_add(evb, ptr, toWrite);
  return toWrite;
}

/* Callback used for the /dump URI, and for every non-GET request:
 * dumps all information to stdout and gives back a trivial 200 ok */
static void
fetch_request_cb(struct evhttp_request *req, void *arg)
{
  struct evkeyvalq    args;
	const char *uri = evhttp_request_get_uri(req);
  evhttp_parse_query(uri, &args);
  char* url = (char *)evhttp_find_header(&args, "url");
  
	struct evbuffer *evb = NULL;
  if (url && strlen(url) > 0) {
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if (curl) {
	    evb = evbuffer_new();
      curl_easy_setopt(curl, CURLOPT_URL, url);
      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, evb);
      res = curl_easy_perform(curl);
      curl_easy_cleanup(curl);
      // this should probably use the curlcode status
      evhttp_send_reply(req, 200, "OK", evb);
      evbuffer_free(evb);
    }
  } else {
    evhttp_send_reply(req, 500, "MEH", NULL);
  }
}

/* Callback used for the /dump URI, and for every non-GET request:
 * dumps all information to stdout and gives back a trivial 200 ok */
static void
s2info_request_cb(struct evhttp_request *req, void *arg)
{

  struct evkeyvalq    args;
  struct evbuffer *inputBuffer = evhttp_request_get_input_buffer (req);
  size_t record_len = evbuffer_get_length(inputBuffer);
  char *postData = NULL;
  const char *path = "/?";
  if (record_len > 0) { 
    postData = (char *)malloc(strlen(path) + record_len + 10);
    postData[0] = '/';
    postData[1] = '?';
    evbuffer_pullup(inputBuffer, -1);                                                                                                                                                     
    evbuffer_copyout(inputBuffer, (char *)postData + strlen(path), record_len);
    postData[strlen(path) + record_len] = '\0';
    printf("trying to parse: %s\n", (const char *)postData);
    evhttp_parse_query((const char *)postData, &args);
  } else {
    const char *uri = evhttp_request_get_uri(req);
    evhttp_parse_query(uri, &args);
  }

  char* callback = (char *)evhttp_find_header(&args, "callback");



  char* ids = (char *)evhttp_find_header(&args, "id");
  std::vector<S2CellId> cellids_vector;
 
  if (ids != NULL) {
    printf("%s\n", ids);
    std::vector<std::string> ids_vector = split(string(ids), ',');
    bool treat_as_tokens = false;
    int num_evenly_divisible = 0;
    for (int i = 0; i < ids_vector.size(); i++) {
      const char *str = ids_vector[i].c_str();
      errno = 0;    /* To distinguish success/failure after call */
      char *endptr;
      uint64_t id = strtoull(str, &endptr, 10);

      if (strlen(endptr) != 0) {
        printf("failed to parse as long long, treating everything as tokens\n");
        treat_as_tokens = true;
      }
      else if (id % 1000 == 0) {
        printf("was even divisible by 1000, assume this was a misinterpreted token\n");
        num_evenly_divisible += 1;
      }
    }

    if (num_evenly_divisible == ids_vector.size()) {
      treat_as_tokens = true;
    }
   
    for (int i = 0; i < ids_vector.size(); i++) {
      const char *str = ids_vector[i].c_str();
      errno = 0;    /* To distinguish success/failure after call */
      char *endptr;
      uint64_t id = strtoull(str, &endptr, 10);
      /*if (str[0] != '-') {
        id = strtoul(str, &endptr, 10);
      }*/

      printf("endptr %d\n", strlen(endptr));
      printf("str %s\n", str);
      printf("id %lld\n", id);

      if (strlen(endptr) != 0 || treat_as_tokens) {
        printf("failed to parse as long long\n");
        cellids_vector.push_back(S2CellId(S2CellId::FromToken(str).id()));
      } else {
        printf("%lld\n", id);
        printf("id != 0 ? %d -- %s %d\n", (id != 0), str, strlen(str));
        printf("is_valid? %d\n", S2CellId(id).is_valid());
        cellids_vector.push_back(S2CellId(id));
      } 
    }
  }

	evhttp_add_header(evhttp_request_get_output_headers(req),
		    "Content-Type", "application/json");
	struct evbuffer *evb = NULL;
	evb = evbuffer_new();
  char* json = s2CellIdsToJson(callback, cellids_vector);
  evbuffer_add_printf(evb, "%s", json);
	evhttp_send_reply(req, 200, "OK", evb);
 
  free(json);

  if (evb)
    evbuffer_free(evb);
}

int
main(int argc, char **argv)
{
	struct event_base *base;
	struct evhttp *http;
	struct evhttp_bound_socket *handle;

	unsigned short port = atoi(argv[1]);
#ifdef WIN32
	WSADATA WSAData;
	WSAStartup(0x101, &WSAData);
#else
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		return (1);
#endif
	base = event_base_new();
	if (!base) {
		fprintf(stderr, "Couldn't create an event_base: exiting\n");
		return 1;
	}

	/* Create a new evhttp object to handle requests. */
	http = evhttp_new(base);
	if (!http) {
		fprintf(stderr, "couldn't create evhttp. Exiting.\n");
		return 1;
	}

	evhttp_set_cb(http, "/s2cover", s2cover_request_cb, NULL);
	evhttp_set_cb(http, "/s2info", s2info_request_cb, NULL);
	evhttp_set_cb(http, "/fetch", fetch_request_cb, NULL);

	/* Now we tell the evhttp what port to listen on */
	handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", port);
	if (!handle) {
		fprintf(stderr, "couldn't bind to port %d. Exiting.\n",
		    (int)port);
		return 1;
	}

	{
		/* Extract and display the address we're listening on. */
		struct sockaddr_storage ss;
		evutil_socket_t fd;
		ev_socklen_t socklen = sizeof(ss);
		char addrbuf[128];
		void *inaddr;
		const char *addr;
		int got_port = -1;
		fd = evhttp_bound_socket_get_fd(handle);
		memset(&ss, 0, sizeof(ss));
		if (getsockname(fd, (struct sockaddr *)&ss, &socklen)) {
			perror("getsockname() failed");
			return 1;
		}
		if (ss.ss_family == AF_INET) {
			got_port = ntohs(((struct sockaddr_in*)&ss)->sin_port);
			inaddr = &((struct sockaddr_in*)&ss)->sin_addr;
		} else if (ss.ss_family == AF_INET6) {
			got_port = ntohs(((struct sockaddr_in6*)&ss)->sin6_port);
			inaddr = &((struct sockaddr_in6*)&ss)->sin6_addr;
		} else {
			fprintf(stderr, "Weird address family %d\n",
			    ss.ss_family);
			return 1;
		}
		addr = evutil_inet_ntop(ss.ss_family, inaddr, addrbuf,
		    sizeof(addrbuf));
		if (addr) {
			printf("HI Listening on %s:%d\n", addr, got_port);
			evutil_snprintf(uri_root, sizeof(uri_root),
			    "http://%s:%d",addr,got_port);
		} else {
			fprintf(stderr, "evutil_inet_ntop failed\n");
			return 1;
		}
	}

	event_base_dispatch(base);

	return 0;
}
