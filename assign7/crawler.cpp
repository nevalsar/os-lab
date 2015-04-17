#include "htmlcxx/html/ParserDom.h"
#include <curl/curl.h>
#include <cstdio>
#include <cstdlib>
#include <htmlcxx/html/ParserDom.h>
#include <iostream>
#include <list>
#include <pthread.h>
#include <queue>
#include <set>
#include <string>
#include <unistd.h>

using namespace std;
using namespace htmlcxx;

#define MASTER_URL "http://cse.iitkgp.ac.in"
#define SPIDER_COUNT 5

typedef struct urlstruct {

	string urlname;
	pthread_t thread_id;
	int depth;
}URL;

size_t write_to_string(void*,size_t,size_t,void*);
int spawn_threads();
void* crawl(void*);
int level_update();
int extract_URLs(URL&,queue<URL>&);
int print_result();

int final_level;
pthread_t spiders[SPIDER_COUNT];
set<string> url_list;

int level;
pthread_mutex_t level_mutex;
queue<URL> to_do;
pthread_mutex_t to_do_mutex;
queue<URL> done_list;
pthread_mutex_t done_mutex;
queue<URL> to_do_next;
pthread_mutex_t to_do_next_mutex;

int main(void){

	URL master;

	fscanf(stdin,"%10d",&final_level);

	level = 1;

	master.urlname = MASTER_URL;
	master.thread_id = pthread_self();
	master.depth = 1;

	to_do.push(master);

	spawn_threads();
	print_result();
	pthread_exit(NULL);

	return 0;
}

int spawn_threads(){

	for(int i = 0;i < SPIDER_COUNT; i++){
		if(pthread_create(&spiders[i],NULL,&crawl,NULL)){
			perror("thread spawned");
			exit(1);
		}
	}
	for(int i = 0;i < SPIDER_COUNT; i++){
		pthread_join(spiders[i], NULL);
	}

	return 0;
}

void* crawl(void*){

	while(1){
		URL current_URL;

		if(to_do.empty())
			level_update();

		current_URL = to_do.front();
		to_do.pop();

		if(url_list.find(current_URL.urlname) != url_list.end())
			continue;

		extract_URLs(current_URL,to_do_next);

		url_list.insert(current_URL.urlname);
		done_list.push(current_URL);
	}
	return NULL;
}

int level_update(){

	if(level < final_level){
		level++;
		while(!to_do_next.empty()){
			to_do.push(to_do_next.front());
			to_do_next.pop();
		}
	}
	else{
		pthread_exit(NULL);
	}
	return 0;
}

int extract_URLs(URL& url,queue<URL>& queue){


	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.urlname.c_str());

		std::string response;
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

    // The "response" variable should now contain the contents of the HTTP response
		HTML::ParserDom parser;
		tree<HTML::Node> dom = parser.parseTree(response);

  //Dump all links in the tree
		tree<HTML::Node>::iterator it = dom.begin();
		tree<HTML::Node>::iterator end = dom.end();
		for (; it != end; ++it){
			if (it->tagName() == "a"){
				it->parseAttributes();
				std::map<std::string, std::string> pairs = it->attributes();
				for ( std::map<std::string, std::string>::const_iterator iter = pairs.begin();
					iter != pairs.end(); ++iter ) {
					if(iter->first.compare("href") == 0){
						string httplink("http://");
						if (std::mismatch(httplink.begin(), httplink.end(), iter->second.begin()).first == httplink.end())
						{
							URL newURL;
							newURL.urlname = iter->second;
							newURL.thread_id = pthread_self();
							newURL.depth = level;
							queue.push(newURL);
						}
					}
				}
			}
		}
	}

	return 0;
}

size_t write_to_string(void *ptr, size_t size, size_t count, void *stream) {
	((std::string*)stream)->append((char*)ptr, 0, size*count);
	return size*count;
}

int print_result(){
	while(done_list.empty()){
		fprintf(stdout,"%30s %6ld %3d",done_list.front().urlname.c_str(),done_list.front().thread_id,done_list.front().depth);
		done_list.pop();
	}
	return 0;
}