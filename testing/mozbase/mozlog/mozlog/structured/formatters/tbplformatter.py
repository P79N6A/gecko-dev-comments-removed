



from .base import BaseFormatter

class TbplFormatter(BaseFormatter):
    """Formatter that formats logs in the legacy formatting format used by TBPL
    This is intended to be used to preserve backward compatibility with existing tools
    hand-parsing this format.
    """
    def __init__(self):
        self.suite_start_time = None
        self.test_start_times = {}

    def __call__(self, data):
        return getattr(self, data["action"])(data)

    def log(self, data):
        if data.get('component'):
            message = "%s %s" % (data["component"], data["message"])
        else:
            message = data["message"]

        if "stack" in data:
            message += "\n%s" % data["stack"]

        return "%s\n" % message

    def process_output(self, data):
        return "PROCESS | %(process)s | %(data)s\n" % data

    def crash(self, data):
        id = self.id_str(data["test"]) if "test" in data else "pid: " % data["process"]

        rv = ["PROCESS-CRASH | %s | application crashed [%s]" % (id,
                                                                 data["signature"])]
        if data.get("minidump_path"):
            rv.append("Crash dump filename: %s" % data["minidump_path"])

        if data.get("stackwalk_stderr"):
            rv.append("stderr from minidump_stackwalk:")
            rv.append(data["stackwalk_stderr"])
        elif data.get("stackwalk_stdout"):
            rv.append(data["stackwalk_stdout"])

        if data.get("stackwalk_returncode", 0) != 0:
            rv.append("minidump_stackwalk exited with return code %d" %
                      data["stackwalk_returncode"])

        if data.get("stackwalk_errors"):
            rv.extend(data.get("stackwalk_errors"))

        rv = "\n".join(rv)
        if not rv[-1] == "\n":
            rv += "\n"

        return rv

    def suite_start(self, data):
        self.suite_start_time = data["time"]
        return "SUITE-START | Running %i tests\n" % len(data["tests"])

    def test_start(self, data):
        self.test_start_times[self.test_id(data["test"])] = data["time"]

        return "TEST-START | %s\n" % self.id_str(data["test"])

    def test_status(self, data):
        message = "- " + data["message"] if "message" in data else ""
        if "stack" in data:
            message += "\n%s" % data["stack"]
        if "expected" in data:
            failure_line = "TEST-UNEXPECTED-%s | %s | %s %s" % (
                data["status"], self.id_str(data["test"]), data["subtest"],
                message)
            info_line = "TEST-INFO | expected %s\n" % data["expected"]
            return "\n".join([failure_line, info_line])

        return "TEST-%s | %s | %s %s\n" % (
            data["status"], self.id_str(data["test"]), data["subtest"],
            message)

    def test_end(self, data):
        test_id = self.test_id(data["test"])
        time_msg = ""
        message = data.get("message", "")
        if "stack" in data:
            message += "\n%s" % data["stack"]
        if test_id in self.test_start_times:
            start_time = self.test_start_times.pop(test_id)
            time = data["time"] - start_time
            time_msg = " | took %ims" % time

        if "expected" in data:
            failure_line = "TEST-UNEXPECTED-%s | %s | %s" % (
                data["status"], test_id, message)

            info_line = "TEST-INFO expected %s%s\n" % (data["expected"], time_msg)
            return "\n".join([failure_line, info_line])

        return "TEST-%s | %s%s\n" % (
            data["status"], test_id, time_msg)

    def suite_end(self, data):
        start_time = self.suite_start_time
        time = int((data["time"] - start_time) / 1000)

        return "SUITE-END | took %is\n" % time

    def test_id(self, test_id):
        if isinstance(test_id, (str, unicode)):
            return test_id
        else:
            return tuple(test_id)

    def id_str(self, test_id):
        if isinstance(test_id, (str, unicode)):
            return test_id
        else:
            return " ".join(test_id)
