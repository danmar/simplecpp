import os
import subprocess
import json

def __run_subprocess(args, env=None, cwd=None, timeout=None):
    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env, cwd=cwd)

    try:
        stdout, stderr = p.communicate(timeout=timeout)
        return_code = p.returncode
        p = None
    except subprocess.TimeoutExpired:
        import psutil
        # terminate all the child processes
        child_procs = psutil.Process(p.pid).children(recursive=True)
        if len(child_procs) > 0:
            for child in child_procs:
                child.terminate()
            try:
                # call with timeout since it might be stuck
                p.communicate(timeout=5)
                p = None
            except subprocess.TimeoutExpired:
                pass
        raise
    finally:
        if p:
            # sending the signal to the process groups causes the parent Python process to terminate as well
            #os.killpg(os.getpgid(p.pid), signal.SIGTERM)  # Send the signal to all the process groups
            p.terminate()
            stdout, stderr = p.communicate()
            p = None

    stdout = stdout.decode(encoding='utf-8', errors='ignore').replace('\r\n', '\n')
    stderr = stderr.decode(encoding='utf-8', errors='ignore').replace('\r\n', '\n')

    return return_code, stdout, stderr

def simplecpp(args = [], cwd = None):
    dir_path = os.path.dirname(os.path.realpath(__file__))
    if 'SIMPLECPP_EXE_PATH' in os.environ:
        simplecpp_path = os.environ['SIMPLECPP_EXE_PATH']
    else:
        simplecpp_path = os.path.join(dir_path, "simplecpp")

    return __run_subprocess([simplecpp_path] + args, cwd = cwd)

def quoted_string(s):
    return json.dumps(str(s))

def format_include_path_arg(include_path):
    return f"-I{str(include_path)}"

def format_isystem_path_arg(include_path):
    return f"-isystem{str(include_path)}"

def format_framework_path_arg(framework_path):
    return f"-F{str(framework_path)}"

def format_iframework_path_arg(framework_path):
    return f"-iframework{str(framework_path)}"

def format_include(include, is_sys_header=False):
    if is_sys_header:
        return f"<{quoted_string(include)[1:-1]}>"
    else:
        return quoted_string(include)
