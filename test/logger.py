from datetime import datetime


class Logger:

    def write_line(self, kind, message):
        message_line = '{0}[{1}]: {2}'.format(str(datetime.now().strftime('%Y-%m-%d %H:%M:%S')), kind,  message)
        print(message_line)

    def write_error(self, essage):
        self.write_line("ERR ", message)

    def write_info(self, message):
        self.write_line("INFO", message)
