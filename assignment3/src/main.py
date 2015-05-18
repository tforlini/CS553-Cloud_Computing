from google.appengine.api import memcache
from StorageApi import StorageApi
from gaesessions import get_current_session
import cgi
import cloudstorage as gcs
import logging
import webapp2
import os
import re
import time
from random import shuffle
from google.appengine.ext import blobstore
from google.appengine.ext.webapp import blobstore_handlers

memcache_client = memcache.Client()
api = StorageApi("cs553-dataset", memcache_client)
html = """
<htmL>
  <head>
      <link rel="stylesheet" href="/stylesheets/knacss.css" />
      <link rel="stylesheet" href="/stylesheets/main.css" />
    <meta charset="utf-8"/>
    <title>Distributed File Storage</title>
  </head>
  <body>
      <div class="line">
        <h1>Distributed File Storage</h1>
        <aside class="mod left w30">
        <h2>Settings</h2>
        <form action="/" method="post">
            <p>Number of threads:
            <input type="radio" name="nthreads" value="1" %s> 1
            <input type="radio" name="nthreads" value="4" %s> 4</p>
            <p>Use memcache: 
            <input type="radio" name="use_memcache" value="True" %s> Yes
            <input type="radio" name="use_memcache" value="False" %s> No</p>
            <input type="submit" value="Apply Settings" />
        </form>
        <h2>Benchmarks</h2>
        <form action="/find_benchmark" method="post">
            <input type="submit" value="Find Benchmark" />
        </form>
        <form action="/delete_benchmark" method="post">
            <input type="submit" value="Delete Benchmark" />
        </form>
        %s
        </aside>
        <aside class="mod left w30">
        <h2>Basic Operations</h2>
        <form enctype="multipart/form-data" action="%s" method="post">
            <label for="file_insert">File insert:</label>
            <input name="file_insert" type="file" value="" multiple/>
            <input name="" type="submit" value="Submit"/>
        </form>
        <form method="post" action="/check">
            <label for="file_check">File check:</label>
            <input name="file_check" type="text" value=""/>
            <input name="" type="submit" value="Submit"/>
        </form>
        <form method="post" action="/find">
            <label for="file_find">File find:</label>
            <input name="file_find" type="text" value=""/>
            <input name="" type="submit" value="Submit"/>
        </form> 
        <form method="post" action="/remove">
            <label for="file_remove">Files remove:</label>
            <input name="file_remove" type="text" value=""/>
            <input name="" type="submit" value="Submit"/>
        </form>
        <form method="post" action="/listing">
            <input name="" type="submit" value="Files Listing"/> 
        </form> 
        </aside>
        <aside class="mod left w40">
        <h2>Extra Credit Operations</h2>
        <form method="post" action="/check_storage">
            <label for="file_check_storage">Check Storage:</label>
            <input name="file_check_storage" type="text" value=""/>
            <input name="" type="submit" value="Submit"/>
        </form>
        <form method="post" action="/check_cache">
            <label for="file_check_cache">Check cache:</label>
            <input name="file_check_cache" type="text" value=""/>
            <input name="" type="submit" value="Submit"/>
        </form>
        <form method="post" action="/remove_all_cache">
            <input name="" type="submit" value="Remove all cache"/> 
        </form> 
        <form method="post" action="/remove_all">
            <input name="" type="submit" value="Remove all files"/> 
        </form>  
        <form method="post" action="/cache_size_mb">
            <input name="" type="submit" value="Cache size (MB)"/> 
        </form>
        <form method="post" action="/cache_size_elem">
            <input name="" type="submit" value="Cache size (elements)"/> 
        </form>
        <form method="post" action="/storage_size_mb">
            <input name="" type="submit" value="Storage size (MB)"/> 
        </form>
        <form method="post" action="/storage_size_elem">
            <input name="" type="submit" value="Storage size (elements)"/> 
        </form>
        <form method="post" action="/find_in_file">
            <label for="file_find_in_file">Find in file:</label>
            <input name="file_find_in_file" type="text" value=""/><br/>
            String: <input name="string_find_in_file" type="text" value=""/>
            <input name="" type="submit" value="Submit"/>
        </form>
        <form method="post" action="/listing_regex">
            Regex: <input name="string_regex" type="text" value=""/>
            <input name="" type="submit" value="Submit"/>
        </form>
        </aside>
     </div>
  </body>
</html>
"""

def get_settings():
    sess = get_current_session()
    # [1 thread, 4 threads, use_memcache]
    settings = ["checked" if sess.get("nthreads", 1) == 1 else "",
                "checked" if sess.get("nthreads", 1) == 4 else "",
                "checked" if sess.get("use_memcache", True) else "",
                "checked" if not sess.get("use_memcache", True) else ""]
    return settings


def display_results(handler, results):
    upload_url = blobstore.create_upload_url('/insert')
    handler.response.write(html % tuple(get_settings() + [results, upload_url]))

class MainPage(webapp2.RequestHandler):
    def get(self):
        upload_url = blobstore.create_upload_url('/insert')
        self.response.write(html % tuple(get_settings() + [" ", upload_url]))

    def post(self):
        sess = get_current_session()
        sess['nthreads'] = int(self.request.POST['nthreads'])
        sess['use_memcache'] = True if self.request.POST['use_memcache'] == "True" else False
        if sess['use_memcache']:
            api.client = memcache.Client()
        else:
            api.client = None
        self.get()

class FindBenchmark(webapp2.RequestHandler):
    def post(self):
    	t0 = time.time()
        sess = get_current_session()
        files = map(lambda x: re.sub(api.bucket_name, "", x[0]), api.listing())
        rand_two_access = files + files
        shuffle(rand_two_access)
        return_values = api.multi_find(rand_two_access, sess.get("nthreads", 1))
        t1 = time.time()
        total = t1-t0
        results = "<h2>Find Benchmark:</h2>Execution time: %fs<ul>" % total
        for r in return_values:
            results += "<li>file %s found: %s</li>" % r
        results += "</ul>"
        display_results(self, results)

class DeleteBenchmark(webapp2.RequestHandler):
    def post(self):
    	t0 = time.time()
        sess = get_current_session()
        files = map(lambda x: re.sub(api.bucket_name, "", x[0]), api.listing())
        return_values = api.multi_remove(files, sess.get("nthreads", 1))
        t1 = time.time()
        total = t1-t0
        results = "<h2>Delete Benchmark:</h2>Execution time: %fs<ul>" % total
        for r in return_values:
            results += "<li>file %s deleted: %s</li>" % r
        results += "</ul>"
        display_results(self, results)

class Insert(blobstore_handlers.BlobstoreUploadHandler):
    def post(self):
    	t0 = time.time()
        sess = get_current_session()
        upload_files = self.get_uploads("file_insert")
        return_values = api.multi_insert(upload_files, sess.get("nthreads", 1))
        t1 = time.time()
        total = t1-t0
        results = "<h2>Insert</h2>Execution time: %fs<ul>" % total
        for r in return_values:
               results += "<li>inserted %s</li>" % r[0].filename
        results += "</ul>"
        display_results(self, results)

class Check(webapp2.RequestHandler):
    def post(self):
        file_check = self.request.POST['file_check']
        check = api.check(file_check)
        results = "<h2>Check</h2><p>File %s found: %s</p>" % (file_check, check)
        display_results(self, results)

class Find(webapp2.RequestHandler):
    def post(self):
        file_find = self.request.get('file_find')
        find = api.find(file_find)
        results = "<h2>Find</h2>"
        if find:
            results += "<p>%s content: %s<br/>...<br/>%s</p><p>total size: %dB</p>" % (file_find, find[:300], find[-300:], len(find))
        else:
            results += "File %s not found" % file_find
        display_results(self, results)

class Remove(webapp2.RequestHandler):
    def post(self):
        file_remove = self.request.get('file_remove')
        remove = api.remove(file_remove)
        results = "<h2>Remove</h2>"
        results += "<p>File %s has been successfully removed: %s</p>" % (file_remove, remove)
        display_results(self, results)

class Listing(webapp2.RequestHandler):
    def post(self):
        file_listing = self.request.get('file_listing')
        listing = api.listing()
        results = "<h2>Listing (%d files)</h2><ul>" % len(listing)
        for l in listing:
            results += "<li>%s (%ld B)</li>" % l
        results += "</ul>"
        display_results(self, results)

class Check_storage(webapp2.RequestHandler):
    def post(self):
        file_check_storage = self.request.get('file_check_storage')
        check_storage = api.check_storage(file_check_storage)
        results = "<h2>Check Storage</h2>"
        results += "<p>%s found in storage: %s</p>" % (file_check_storage, check_storage)
        display_results(self,results)    

class Check_cache(webapp2.RequestHandler):
    def post(self):
        file_check_cache = self.request.get('file_check_cache')
        check_cache= api.check_cache(file_check_cache)
        results = "<h2>Check Cache</h2>"
        results += "<p>%s found in cache: %s</p>" % (file_check_cache, check_cache)
        display_results(self,results)

class Remove_all_cache(webapp2.RequestHandler):
    def post(self):
        file_remove_all_cache = self.request.get('file_remove_all_cache')
        remove_all_cache= api.remove_all_cache()
        results = "<h2>Remove All Cache</h2>"
        results += "<p>The cache has been emptied successfully: %s</p>" % remove_all_cache
        display_results(self, results)

class Remove_all(webapp2.RequestHandler):
    def post(self):
        file_remove_all = self.request.get('file_remove_all')
        remove_all = api.remove_all()
        results = "<h2>Remove All</h2>"
        results += "<p>All files have been removed successfully: %s</p>" % remove_all 
        display_results(self, results)

class Cache_size_mb(webapp2.RequestHandler):
    def post(self):
        file_cache_size_mb = self.request.get('file_cache_size_mb')
        cache_size_mb = api.cache_size_mb()
        results = "<h2>Cache size</h2><p>%f MB</p>" % cache_size_mb
        display_results(self, results)

class Cache_size_elem(webapp2.RequestHandler):
    def post(self):
        file_cache_size_elem = self.request.get('file_cache_size_elem')
        cache_size_elem = api.cache_size_elem()
        results = "<h2>Number of elements in the cache</h2><p>%d elements</p>" % cache_size_elem
        display_results(self, results)

class Storage_size_mb(webapp2.RequestHandler):
    def post(self):
        file_storage_size_mb = self.request.get('file_storage_size_mb')
        storage_size_mb = api.storage_size_mb()
        results = "<h2>Storage size</h2><p>%f MB</p>" % storage_size_mb
        display_results(self, results)

class Storage_size_elem(webapp2.RequestHandler):
    def post(self):
        file_storage_size_elem = self.request.get('file_storage_size_elem')
        storage_size_elem = api.storage_size_elem()
        results = "<h2>Number of elements in the storage</h2><p>%d elements</p>" % storage_size_elem
        display_results(self, results)

class Find_in_file(webapp2.RequestHandler):
    def post(self):
        file_find_in_file= self.request.get('file_find_in_file')
        string_find_in_file = self.request.get('string_find_in_file')
        find_in_file = api.find_in_file(file_find_in_file, string_find_in_file)
        results = "<h2>Find in file</h2>"
        results += "Found %s in %s: %s" % (string_find_in_file, file_find_in_file, find_in_file)

class Listing_regex(webapp2.RequestHandler):
    def post(self):
        string_regex = self.request.get('string_regex')
        listing_regex = api.listing(string_regex)
        results = "<h2>Files matching %s:</h2>" % string_regex
        for l in listing_regex:
            results += "<li>%s (%ld B)</li>" % l
        results += "</ul>"
        display_results(self, results)

application = webapp2.WSGIApplication([
    ('/', MainPage),
    ('/find_benchmark', FindBenchmark),
    ('/delete_benchmark', DeleteBenchmark),
    ('/insert', Insert),
    ('/check', Check),
    ('/find', Find),
    ('/remove', Remove),
    ('/listing', Listing),
    ('/check_storage', Check_storage),
    ('/check_cache', Check_cache),
    ('/remove_all_cache', Remove_all_cache),
    ('/remove_all', Remove_all),
    ('/cache_size_mb', Cache_size_mb),
    ('/cache_size_elem', Cache_size_elem),
    ('/storage_size_mb', Storage_size_mb),
    ('/storage_size_elem', Storage_size_elem),
    ('/find_in_file', Find_in_file),
    ('/listing_regex', Listing_regex),
], debug=True)
