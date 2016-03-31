# Copyright (C) 2003-2016 XMMS2 Team
#
# As sphinx doesn't detect constants in modules we have to
# generate parts of the documentation. To keep it all in the
# same place, all of the documentation sources are generated.

from __future__ import with_statement
import ast
import os
import sys
import subprocess

try:
    sys.path.remove(os.path.dirname(os.path.abspath(__file__)))
    import xmmsclient
    import xmmsclient.collections
except:
    print("Set PYTHONPATH to a path containing xmmsclient.")
    raise SystemExit

def write_xmmsapi(filename):
    with open(filename, "w") as dst:
        dst.write("xmmsclient\n")
        dst.write("----------\n")
        dst.write("See `samples` on how to connect and interact with the server.\n\n")
        dst.write("Classes\n")
        dst.write("^^^^^^^\n")
        dst.write(".. autoclass:: xmmsclient.Xmms\n\n")
        dst.write(".. autoclass:: xmmsclient.XmmsSync\n")
        dst.write(".. autoclass:: xmmsclient.xmmsapi.XmmsCore\n")
        dst.write(".. autoclass:: xmmsclient.xmmsapi.XmmsApi\n")
        dst.write(".. autoclass:: xmmsclient.XmmsValue\n")
        dst.write(".. autoclass:: xmmsclient.XmmsValueC2C\n")
        dst.write(".. autoclass:: xmmsclient.XmmsLoop\n")
        dst.write(".. autoclass:: xmmsclient.xmmsapi.XmmsResult\n")
        dst.write(".. autoclass:: xmmsclient.xmmsapi.XmmsVisChunk\n")
        dst.write("Exceptions\n")
        dst.write("^^^^^^^^^^\n")
        dst.write(".. autoclass:: xmmsclient.xmmsvalue.XmmsError\n")
        dst.write(".. autoclass:: xmmsclient.xmmsapi.VisualizationError\n")
        dst.write(".. autoclass:: xmmsclient.xmmsapi.XmmsDisconnectException\n")

def write_service_clients(filename):
    with open(filename, "w") as dst:
        dst.write("xmmsclient.service\n")
        dst.write("------------------\n\n")
        dst.write("See `samples` on how to register and/or consume service clients.\n\n")
        dst.write("Classes\n")
        dst.write("^^^^^^^\n")
        dst.write(".. autoclass:: xmmsclient.service.XmmsServiceClient\n")
        dst.write(".. autoclass:: xmmsclient.xmmsapi.XmmsServiceNamespace\n")
        dst.write("   :no-members:\n\n")
        dst.write("   .. automethod:: xmmsclient.xmmsapi.XmmsServiceNamespace.register\n")
        dst.write("Decorators\n")
        dst.write("^^^^^^^^^^\n")
        dst.write(".. py:decorator:: xmmsclient.service.method_arg\n")
        dst.write(".. py:decorator:: xmmsclient.service.method_varg\n")
        dst.write(".. py:decorator:: xmmsclient.service.service_method\n")
        dst.write(".. py:decorator:: xmmsclient.service.service_broadcast\n")
        dst.write(".. py:decorator:: xmmsclient.service.service_constant\n")
        dst.write(".. py:decorator:: xmmsclient.service.client_broadcast\n")
        dst.write(".. py:decorator:: xmmsclient.service.client_method\n")

def write_constants(filename):
    with open(xmmsclient.consts.__file__) as src, open(filename, "w") as dst:
        class ConstsDocumenter(ast.NodeVisitor):
            def visit_ImportFrom(self, node):
                for name in node.names:
                    dst.write(".. autoattribute:: xmmsclient.consts.%s\n" % name.name)
        dst.write("xmmsclient.consts\n")
        dst.write("-----------------\n")
        dst.write("Constants\n")
        dst.write("^^^^^^^^^\n")
        tree = ast.parse(src.read())
        ConstsDocumenter().visit(tree)

def write_collections(filename):
    with open(xmmsclient.collections.__file__) as src, open(filename, "w") as dst:
        class CollectionsDocumenter(ast.NodeVisitor):
            def visit_ImportFrom(self, node):
                for name in node.names:
                    if name.name == "coll_parse":
                        dst.write("Methods\n")
                        dst.write("^^^^^^^\n")
                        dst.write(".. autofunction:: xmmsclient.collections.coll_parse\n")
                    else:
                        dst.write(".. autoclass:: xmmsclient.collections.%s\n" % name.name)
                        dst.write("   :inherited-members:\n\n")
                        dst.write("   .. automethod:: xmmsclient.collections.%s.__init__\n" % name.name)
        dst.write("xmmsclient.collections\n")
        dst.write("----------------------\n")
        dst.write("Classes\n")
        dst.write("^^^^^^^\n")
        tree = ast.parse(src.read())
        CollectionsDocumenter().visit(tree)

def write_samples(filename, samples_dir):
    with open(filename, "w") as dst:
        dst.write("\n")
        dst.write("Examples\n")
        dst.write("--------\n")
        dst.write("Basic example\n")
        dst.write("^^^^^^^^^^^^^\n")
        dst.write(".. code-block:: python\n\n")
        dst.write("   import xmmsclient\n")
        dst.write("   \n")
        dst.write("   xc = xmmsclient.XmmsSync('test-client')\n")
        dst.write("   xc.connect()\n\n")
        dst.write("   print(xc.stats())\n\n")
        dst.write("Async example\n")
        dst.write("^^^^^^^^^^^^^\n")
        dst.write(".. code-block:: python\n\n")
        dst.write("   import xmmsclient\n")
        dst.write("   \n")
        dst.write("   def handle_stats(result):\n")
        dst.write("     print(result.value())\n")
        dst.write("   \n")
        dst.write("   xc = xmmsclient.XmmsLoop('test-client')\n")
        dst.write("   xc.connect()\n")
        dst.write("   xc.stats(cb=handle_stats)\n")
        dst.write("   xc.loop()\n\n")
        dst.write("Browse and track available service clients\n")
        dst.write("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n")
        dst.write(".. literalinclude:: %s\n" % os.path.join(samples_dir, "scwatch.py"))
        dst.write("   :language: python\n")
        dst.write("   :tab-width: 2\n")
        dst.write("\n")
        dst.write("Register a service client\n")
        dst.write("^^^^^^^^^^^^^^^^^^^^^^^^^\n")
        dst.write(".. literalinclude:: %s\n" % os.path.join(samples_dir, "service.py"))
        dst.write("   :language: python\n")
        dst.write("   :tab-width: 2\n")

def write_index(filename):
    with open(filename, "w") as dst:
        dst.write("Welcome to XMMS2's documentation!\n")
        dst.write("=================================\n")

        dst.write(".. toctree::\n")
        dst.write("   :maxdepth: 1\n\n")
        dst.write("   xmmsapi\n")
        dst.write("   constants\n")
        dst.write("   collections\n")
        dst.write("   serviceclients\n")
        dst.write("   samples\n")

def write_conf(filename):
    with open(filename, "w") as dst:
        dst.write("""
project = 'XMMS2'
copyright = '2016, XMMS2 Team'
version = '0.8+WiP'
master_doc = 'index'
html_theme = 'sphinx_py3doc_enhanced_theme'

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.autosummary'
]

autodoc_default_flags = ['members', 'undoc-members', 'show-inheritance' ]
default_role = 'any'
""")

if __name__ == '__main__':
    basedir = "docs/python/src"
    script_dir = os.path.dirname(os.path.abspath(__file__))
    samples_dir = os.path.relpath(os.path.join(script_dir, "xmmsclient", "samples"), basedir)

    try:
        os.makedirs(basedir)
    except:
        pass
    write_xmmsapi(os.path.join(basedir, "xmmsapi.rst"))
    write_constants(os.path.join(basedir, "constants.rst"))
    write_collections(os.path.join(basedir, "collections.rst"))
    write_service_clients(os.path.join(basedir, "serviceclients.rst"))
    write_samples(os.path.join(basedir, "samples.rst"), samples_dir)
    write_index(os.path.join(basedir, "index.rst"))
    write_conf(os.path.join(basedir, "conf.py"))

    cmd = os.environ.get("SPHINXBUILD", "sphinx-build")
    subprocess.call([cmd, "docs/python/src", "docs/python/html"])
