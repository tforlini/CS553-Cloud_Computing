from StorageApi import StorageApi
from google.appengine.api import memcache
from gaesessions import get_current_session
from os import listdir
from os.path import isfile, join, getsize
import time, random, threading, webapp2
from main import *
"""
USAGE :
in  benchmark(self):

fill the variables mem_cache, thread number, path and options

mem_cache : Usage of mem_cache or not, not implemented yet.
thread_number : Choose number of threads, expected values 1 | 4
path : path to a data folder
options : [find,delete] possible values True|False for each field.

TODO - change 25 for num_files (2 occurences)
	
"""



class MainClass(webapp2.RequestHandler):

	memcache_client = memcache_client
	api = api

	def bench_1thread(self,mem_cache, threads,files, options):
	
		find = options[0]
		delete = options[1]
		only_files = [x[0] for x in files]
		#reading
		if find:
			findt1 = time.clock()	
			num_files = len(only_files)
			for i in range(0,2*25):
				rd_file = only_files[random.randrange(0,num_files)]
				filecontent = self.api.find(rd_file)
			findt2 = time.clock()	
			self.response.write('<br /><br />Find - time elapsed '+str(findt2 - findt1) + 's<br />')

		#deleting
		if delete:
			removet1 = time.clock()
			self.api.remove_all()
			removet2 = time.clock()
			self.response.write('<br /><br />Remove - time elapsed '+str(removet2 - removet1) + 's<br />')

	def bench_nthreads(self,mem_cache, thread_number, files, options):
	
		find = options[0]
		delete = options[1]
		only_files = [x[0] for x in files]

		#reading
		if find:
			findt1 = time.clock()	
			num_files = len(only_files)
			for i in range(0,2*25):
				rd_file = only_files[random.randrange(0,num_files)]
				filecontent = self.api.find(only_files[random.randrange(0,num_files)])
			findt2 = time.clock()	
			self.response.write('<br /><br />Th'+str(thread_number)+' Find - time elapsed '+str(findt2 - findt1) + 's<br />')

		#deleting
		if delete:
			removet1 = time.clock()
			for f in only_files:
				a = self.api.remove(f)
				if thread_number == 4:
					self.response.write(f + ' ')
					self.response.write(str(a) + ' ')
			removet2 = time.clock()
			self.response.write('<br /><br />Th'+str(thread_number)+' Remove - time elapsed '+str(removet2 - removet1) + 's<br />')


	"""
	USAGE :
	in  benchmark(self):

	fill the variables mem_cache, thread number, path and options

	mem_cache : Usage of mem_cache or not, not implemented yet.
	thread_number : Choose number of threads, expected values 1 | 4
	path : path to a data folder
	options : [find,delete] possible values True|False for each field.

	"""


	def benchmark(self):


		mem_cache = True 
		thread_number = 4
		path = '/home/thomas/workspace/CloudComputing/cs553-cloudcomputing-2014/assignment3/src/data'
	
		options = [False,True]
	
		self.memcache_client = memcache.Client()
		self.api = StorageApi("cs553-dataset", self.memcache_client)

		# options self.response.writeing
		self.response.write('---- Benchmark ----<br />')
		if mem_cache:	
			self.response.write('memcache ')
		else:
			self.response.write('no_memcache ')
		self.response.write(str(thread_number) + 'threads ')
		if options[0]:
			self.response.write('find ')
		if options[1]:
			self.response.write('delete')
		self.response.write('<br />')

		if mem_cache:
			string = 'mem_cache'
		else:
			string = None

		# processing
		files = self.api.listing()
		if thread_number == 1:
			self.bench_1thread(string, thread_number, files, options)
		elif thread_number == 4:
			num_files = len(files)

			threads = []
			for i in range(0,4):
				t = threading.Thread(target=self.bench_nthreads, args=(string, i+1, files[i*num_files/4:(i+1)*num_files/4], options))
				threads.append(t)
			for i in range(0,4):
				threads[i].start()
			for i in range(0,4):
				threads[i].join()
	   	else:
			self.response.write ('__main__ number of threads not standard<br />')

		#self.response.write('<br />Listing<br />')
		#self.response.write(self.api.listing())
		#self.response.write('<br />Cache Elements<br />')
		#self.response.write(self.api.cache_size_elem())
		#self.response.write('<br />'+str(api.find('51451ygsuq'))+'<br />')
	def get(self):
		self.benchmark()

app = webapp2.WSGIApplication([
    ('/benchmark', MainClass),
], debug=True)

